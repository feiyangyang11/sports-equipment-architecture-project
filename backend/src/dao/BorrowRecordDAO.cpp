#include "dao/BorrowRecordDAO.hpp"

#include <mysql/mysql.h>

#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using MysqlConnection = std::unique_ptr<MYSQL, decltype(&mysql_close)>;
using MysqlResult = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

std::string toString(const char* value) {
  return value != nullptr ? value : "";
}

std::uint64_t toUInt64(const char* value) {
  return value != nullptr ? std::strtoull(value, nullptr, 10) : 0ULL;
}

std::uint32_t toUInt32(const char* value) {
  return value != nullptr
             ? static_cast<std::uint32_t>(std::strtoul(value, nullptr, 10))
             : 0U;
}

BorrowRecordView mapBorrowRecordViewRow(MYSQL_ROW row) {
  BorrowRecordView borrowRecord;
  borrowRecord.id = toUInt64(row[0]);
  borrowRecord.borrowNo = toString(row[1]);
  borrowRecord.reservationId = toUInt64(row[2]);
  borrowRecord.reservationNo = toString(row[3]);
  borrowRecord.studentUserId = toUInt64(row[4]);
  borrowRecord.studentUsername = toString(row[5]);
  borrowRecord.studentRealName = toString(row[6]);
  borrowRecord.equipmentId = toUInt64(row[7]);
  borrowRecord.equipmentCode = toString(row[8]);
  borrowRecord.equipmentName = toString(row[9]);
  borrowRecord.quantity = toUInt32(row[10]);
  borrowRecord.borrowedBy = toUInt64(row[11]);
  borrowRecord.borrowedAt = toString(row[12]);
  borrowRecord.dueAt = toString(row[13]);
  borrowRecord.receivedBy = toUInt64(row[14]);
  borrowRecord.returnedAt = toString(row[15]);
  borrowRecord.status = toString(row[16]);
  borrowRecord.returnNote = toString(row[17]);
  borrowRecord.createdAt = toString(row[18]);
  borrowRecord.updatedAt = toString(row[19]);
  return borrowRecord;
}

MysqlConnection createConnection(const DatabaseConfig& config) {
  MYSQL* rawConnection = mysql_init(nullptr);
  if (rawConnection == nullptr) {
    throw std::runtime_error("mysql_init failed");
  }

  MysqlConnection connection(rawConnection, mysql_close);

  my_bool disableSsl = 0;
  mysql_options(connection.get(), MYSQL_OPT_SSL_ENFORCE, &disableSsl);
  mysql_options(connection.get(), MYSQL_OPT_SSL_VERIFY_SERVER_CERT,
                &disableSsl);

  if (mysql_real_connect(connection.get(), config.host.c_str(),
                         config.username.c_str(), config.password.c_str(),
                         config.database.c_str(), config.port, nullptr, 0) ==
      nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  if (mysql_set_character_set(connection.get(), config.charset.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  return connection;
}

std::string escapeSqlString(MYSQL* connection, const std::string& value) {
  std::string escaped(value.size() * 2 + 1, '\0');
  const auto escapedLength = mysql_real_escape_string(
      connection, escaped.data(), value.c_str(),
      static_cast<unsigned long>(value.size()));
  escaped.resize(escapedLength);
  return escaped;
}

std::string toNullableSqlLiteral(MYSQL* connection, const std::string& value) {
  if (value.empty()) {
    return "NULL";
  }
  return "'" + escapeSqlString(connection, value) + "'";
}

void executeSql(MYSQL* connection, const std::string& sql) {
  if (mysql_query(connection, sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection));
  }
}

std::string buildStudentAndStatusFilter(MYSQL* connection,
                                        std::uint64_t studentUserId,
                                        std::optional<std::string> status) {
  std::string filter =
      " WHERE b.student_user_id = " + std::to_string(studentUserId);
  if (status.has_value()) {
    filter += " AND b.status = '" +
              escapeSqlString(connection, status.value()) + "'";
  }
  return filter;
}

std::string buildStatusFilter(MYSQL* connection,
                              std::optional<std::string> status) {
  if (!status.has_value()) {
    return "";
  }

  return " WHERE b.status = '" + escapeSqlString(connection, status.value()) +
         "'";
}

std::string buildBorrowViewSelectSql(const std::string& whereAndOrderClause) {
  return "SELECT b.id, b.borrow_no, b.reservation_id, r.reservation_no, "
         "b.student_user_id, u.username, u.real_name, b.equipment_id, "
         "e.equipment_code, e.equipment_name, b.quantity, b.borrowed_by, "
         "b.borrowed_at, b.due_at, b.received_by, b.returned_at, b.status, "
         "b.return_note, b.created_at, b.updated_at "
         "FROM borrow_record b "
         "INNER JOIN reservation r ON r.id = b.reservation_id "
         "INNER JOIN `user` u ON u.id = b.student_user_id "
         "INNER JOIN equipment e ON e.id = b.equipment_id" +
         whereAndOrderClause;
}

bool rollbackQuietly(MYSQL* connection) {
  return mysql_query(connection, "ROLLBACK") == 0;
}

}  // namespace

BorrowRecordDAO::BorrowRecordDAO(DatabaseConfig config)
    : m_config(std::move(config)) {}

std::optional<std::uint64_t> BorrowRecordDAO::createFromApprovedReservation(
    std::uint64_t reservationId,
    std::uint64_t borrowedBy,
    const std::string& borrowedAt,
    const std::string& dueAt) const {
  auto connection = createConnection(m_config);
  executeSql(connection.get(), "START TRANSACTION");

  try {
    const std::string loadSql =
        "SELECT r.id, r.student_user_id, r.equipment_id, r.quantity, r.status, "
        "e.available_stock "
        "FROM reservation r "
        "INNER JOIN equipment e ON e.id = r.equipment_id "
        "WHERE r.id = " +
        std::to_string(reservationId) + " LIMIT 1 FOR UPDATE";

    executeSql(connection.get(), loadSql);
    MysqlResult loadResult(mysql_store_result(connection.get()),
                           mysql_free_result);
    if (loadResult == nullptr) {
      throw std::runtime_error(mysql_error(connection.get()));
    }

    MYSQL_ROW loadRow = mysql_fetch_row(loadResult.get());
    if (loadRow == nullptr) {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    const auto studentUserId = toUInt64(loadRow[1]);
    const auto equipmentId = toUInt64(loadRow[2]);
    const auto quantity = toUInt32(loadRow[3]);
    const auto reservationStatus = toString(loadRow[4]);
    const auto availableStock = toUInt32(loadRow[5]);

    if (reservationStatus != "APPROVED" || quantity > availableStock) {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    std::string borrowNo = "BOR";
    borrowNo += borrowedAt.substr(0, 4);
    borrowNo += borrowedAt.substr(5, 2);
    borrowNo += borrowedAt.substr(8, 2);
    borrowNo += borrowedAt.substr(11, 2);
    borrowNo += borrowedAt.substr(14, 2);
    borrowNo += borrowedAt.substr(17, 2);
    borrowNo += std::to_string(reservationId);

    const std::string insertSql =
        "INSERT INTO borrow_record (borrow_no, reservation_id, student_user_id, "
        "equipment_id, quantity, borrowed_by, borrowed_at, due_at, status) "
        "VALUES ('" +
        escapeSqlString(connection.get(), borrowNo) + "', " +
        std::to_string(reservationId) + ", " + std::to_string(studentUserId) +
        ", " + std::to_string(equipmentId) + ", " + std::to_string(quantity) +
        ", " + std::to_string(borrowedBy) + ", '" +
        escapeSqlString(connection.get(), borrowedAt) + "', '" +
        escapeSqlString(connection.get(), dueAt) + "', 'BORROWING')";
    executeSql(connection.get(), insertSql);

    const auto borrowRecordId =
        static_cast<std::uint64_t>(mysql_insert_id(connection.get()));

    const std::string updateReservationSql =
        "UPDATE reservation SET status = 'BORROWED' "
        "WHERE id = " + std::to_string(reservationId) +
        " AND status = 'APPROVED'";
    executeSql(connection.get(), updateReservationSql);
    if (mysql_affected_rows(connection.get()) != 1) {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    const std::string updateEquipmentSql =
        "UPDATE equipment SET available_stock = available_stock - " +
        std::to_string(quantity) + " WHERE id = " +
        std::to_string(equipmentId) + " AND available_stock >= " +
        std::to_string(quantity);
    executeSql(connection.get(), updateEquipmentSql);
    if (mysql_affected_rows(connection.get()) != 1) {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    executeSql(connection.get(), "COMMIT");
    return borrowRecordId;
  } catch (...) {
    rollbackQuietly(connection.get());
    throw;
  }
}

std::optional<std::uint64_t> BorrowRecordDAO::returnBorrowRecord(
    std::uint64_t borrowRecordId,
    std::uint64_t receivedBy,
    const std::string& returnedAt,
    const std::string& returnNote) const {
  auto connection = createConnection(m_config);
  executeSql(connection.get(), "START TRANSACTION");

  try {
    const std::string loadSql =
        "SELECT id, reservation_id, equipment_id, quantity, status "
        "FROM borrow_record WHERE id = " +
        std::to_string(borrowRecordId) + " LIMIT 1 FOR UPDATE";
    executeSql(connection.get(), loadSql);

    MysqlResult loadResult(mysql_store_result(connection.get()),
                           mysql_free_result);
    if (loadResult == nullptr) {
      throw std::runtime_error(mysql_error(connection.get()));
    }

    MYSQL_ROW loadRow = mysql_fetch_row(loadResult.get());
    if (loadRow == nullptr) {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    const auto reservationId = toUInt64(loadRow[1]);
    const auto equipmentId = toUInt64(loadRow[2]);
    const auto quantity = toUInt32(loadRow[3]);
    const auto borrowStatus = toString(loadRow[4]);

    if (borrowStatus != "BORROWING") {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    const std::string updateBorrowSql =
        "UPDATE borrow_record "
        "SET received_by = " + std::to_string(receivedBy) +
        ", returned_at = '" + escapeSqlString(connection.get(), returnedAt) +
        "', status = 'RETURNED', return_note = " +
        toNullableSqlLiteral(connection.get(), returnNote) +
        " WHERE id = " + std::to_string(borrowRecordId) +
        " AND status = 'BORROWING'";
    executeSql(connection.get(), updateBorrowSql);
    if (mysql_affected_rows(connection.get()) != 1) {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    const std::string updateReservationSql =
        "UPDATE reservation SET status = 'COMPLETED' "
        "WHERE id = " + std::to_string(reservationId) +
        " AND status = 'BORROWED'";
    executeSql(connection.get(), updateReservationSql);
    if (mysql_affected_rows(connection.get()) != 1) {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    const std::string updateEquipmentSql =
        "UPDATE equipment SET available_stock = available_stock + " +
        std::to_string(quantity) + " WHERE id = " +
        std::to_string(equipmentId);
    executeSql(connection.get(), updateEquipmentSql);
    if (mysql_affected_rows(connection.get()) != 1) {
      executeSql(connection.get(), "ROLLBACK");
      return std::nullopt;
    }

    executeSql(connection.get(), "COMMIT");
    return borrowRecordId;
  } catch (...) {
    rollbackQuietly(connection.get());
    throw;
  }
}

std::uint64_t BorrowRecordDAO::countMyBorrows(
    std::uint64_t studentUserId, std::optional<std::string> status) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT COUNT(*) FROM borrow_record b" +
      buildStudentAndStatusFilter(connection.get(), studentUserId,
                                  std::move(status));

  executeSql(connection.get(), sql);
  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MYSQL_ROW row = mysql_fetch_row(result.get());
  if (row == nullptr) {
    return 0ULL;
  }

  return toUInt64(row[0]);
}

std::vector<BorrowRecordView> BorrowRecordDAO::queryMyBorrowsPage(
    std::uint64_t studentUserId, std::uint32_t pageNo, std::uint32_t pageSize,
    std::optional<std::string> status) const {
  const std::uint64_t offset =
      static_cast<std::uint64_t>(pageNo - 1) *
      static_cast<std::uint64_t>(pageSize);

  auto connection = createConnection(m_config);
  const std::string sql = buildBorrowViewSelectSql(
      buildStudentAndStatusFilter(connection.get(), studentUserId,
                                  std::move(status)) +
      " ORDER BY b.borrowed_at DESC, b.id DESC LIMIT " +
      std::to_string(pageSize) + " OFFSET " + std::to_string(offset));

  executeSql(connection.get(), sql);
  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  std::vector<BorrowRecordView> borrowRecords;
  MYSQL_ROW row = nullptr;
  while ((row = mysql_fetch_row(result.get())) != nullptr) {
    borrowRecords.push_back(mapBorrowRecordViewRow(row));
  }

  return borrowRecords;
}

std::optional<BorrowRecordView> BorrowRecordDAO::getMyBorrowById(
    std::uint64_t studentUserId, std::uint64_t borrowRecordId) const {
  auto connection = createConnection(m_config);
  const std::string sql = buildBorrowViewSelectSql(
      " WHERE b.id = " + std::to_string(borrowRecordId) +
      " AND b.student_user_id = " + std::to_string(studentUserId) +
      " LIMIT 1");

  executeSql(connection.get(), sql);
  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MYSQL_ROW row = mysql_fetch_row(result.get());
  if (row == nullptr) {
    return std::nullopt;
  }

  return mapBorrowRecordViewRow(row);
}

std::uint64_t BorrowRecordDAO::countAdminBorrows(
    std::optional<std::string> status) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT COUNT(*) FROM borrow_record b" +
      buildStatusFilter(connection.get(), std::move(status));

  executeSql(connection.get(), sql);
  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MYSQL_ROW row = mysql_fetch_row(result.get());
  if (row == nullptr) {
    return 0ULL;
  }

  return toUInt64(row[0]);
}

std::vector<BorrowRecordView> BorrowRecordDAO::queryAdminBorrowsPage(
    std::uint32_t pageNo, std::uint32_t pageSize,
    std::optional<std::string> status) const {
  const std::uint64_t offset =
      static_cast<std::uint64_t>(pageNo - 1) *
      static_cast<std::uint64_t>(pageSize);

  auto connection = createConnection(m_config);
  const std::string sql = buildBorrowViewSelectSql(
      buildStatusFilter(connection.get(), std::move(status)) +
      " ORDER BY b.borrowed_at DESC, b.id DESC LIMIT " +
      std::to_string(pageSize) + " OFFSET " + std::to_string(offset));

  executeSql(connection.get(), sql);
  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  std::vector<BorrowRecordView> borrowRecords;
  MYSQL_ROW row = nullptr;
  while ((row = mysql_fetch_row(result.get())) != nullptr) {
    borrowRecords.push_back(mapBorrowRecordViewRow(row));
  }

  return borrowRecords;
}

std::optional<BorrowRecordView> BorrowRecordDAO::getBorrowById(
    std::uint64_t borrowRecordId) const {
  auto connection = createConnection(m_config);
  const std::string sql = buildBorrowViewSelectSql(
      " WHERE b.id = " + std::to_string(borrowRecordId) + " LIMIT 1");

  executeSql(connection.get(), sql);
  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MYSQL_ROW row = mysql_fetch_row(result.get());
  if (row == nullptr) {
    return std::nullopt;
  }

  return mapBorrowRecordViewRow(row);
}
