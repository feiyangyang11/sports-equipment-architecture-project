#include "dao/ReservationDAO.hpp"

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
  return value != nullptr ? static_cast<std::uint32_t>(std::strtoul(value, nullptr, 10))
                          : 0U;
}

ReservationView mapReservationViewRow(MYSQL_ROW row) {
  ReservationView reservation;
  reservation.id = toUInt64(row[0]);
  reservation.reservationNo = toString(row[1]);
  reservation.studentUserId = toUInt64(row[2]);
  reservation.studentUsername = toString(row[3]);
  reservation.studentRealName = toString(row[4]);
  reservation.equipmentId = toUInt64(row[5]);
  reservation.equipmentCode = toString(row[6]);
  reservation.equipmentName = toString(row[7]);
  reservation.reservationStartAt = toString(row[8]);
  reservation.reservationEndAt = toString(row[9]);
  reservation.quantity = toUInt32(row[10]);
  reservation.status = toString(row[11]);
  reservation.requestNote = toString(row[12]);
  reservation.reviewNote = toString(row[13]);
  reservation.cancelReason = toString(row[14]);
  reservation.reviewedBy = toUInt64(row[15]);
  reservation.reviewedAt = toString(row[16]);
  reservation.createdAt = toString(row[17]);
  reservation.updatedAt = toString(row[18]);
  return reservation;
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

std::string buildStudentAndStatusFilter(
    MYSQL* connection,
    std::uint64_t studentUserId,
    std::optional<std::string> status) {
  std::string filter =
      " WHERE r.student_user_id = " + std::to_string(studentUserId);
  if (status.has_value()) {
    filter += " AND r.status = '" +
              escapeSqlString(connection, status.value()) + "'";
  }
  return filter;
}

std::string buildStatusFilter(MYSQL* connection,
                              std::optional<std::string> status) {
  if (!status.has_value()) {
    return "";
  }

  return " WHERE r.status = '" + escapeSqlString(connection, status.value()) +
         "'";
}

}  // namespace

ReservationDAO::ReservationDAO(DatabaseConfig config)
    : m_config(std::move(config)) {}

std::uint64_t ReservationDAO::create(const Reservation& reservation) const {
  auto connection = createConnection(m_config);

  const std::string sql =
      "INSERT INTO reservation (reservation_no, student_user_id, equipment_id, "
      "reservation_start_at, reservation_end_at, quantity, status, request_note) "
      "VALUES ('" +
      escapeSqlString(connection.get(), reservation.reservationNo) + "', " +
      std::to_string(reservation.studentUserId) + ", " +
      std::to_string(reservation.equipmentId) + ", '" +
      escapeSqlString(connection.get(), reservation.reservationStartAt) + "', '" +
      escapeSqlString(connection.get(), reservation.reservationEndAt) + "', " +
      std::to_string(reservation.quantity) + ", '" +
      escapeSqlString(connection.get(), reservation.status) + "', " +
      toNullableSqlLiteral(connection.get(), reservation.requestNote) + ")";

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  return static_cast<std::uint64_t>(mysql_insert_id(connection.get()));
}

std::uint64_t ReservationDAO::countMyReservations(
    std::uint64_t studentUserId,
    std::optional<std::string> status) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT COUNT(*) FROM reservation r" +
      buildStudentAndStatusFilter(connection.get(), studentUserId,
                                  std::move(status));

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

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

std::vector<ReservationView> ReservationDAO::queryMyReservationsPage(
    std::uint64_t studentUserId,
    std::uint32_t pageNo,
    std::uint32_t pageSize,
    std::optional<std::string> status) const {
  const std::uint64_t offset =
      static_cast<std::uint64_t>(pageNo - 1) *
      static_cast<std::uint64_t>(pageSize);

  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT r.id, r.reservation_no, r.student_user_id, u.username, "
      "u.real_name, r.equipment_id, e.equipment_code, e.equipment_name, "
      "r.reservation_start_at, r.reservation_end_at, r.quantity, r.status, "
      "r.request_note, r.review_note, r.cancel_reason, r.reviewed_by, "
      "r.reviewed_at, r.created_at, r.updated_at "
      "FROM reservation r "
      "INNER JOIN `user` u ON u.id = r.student_user_id "
      "INNER JOIN equipment e ON e.id = r.equipment_id" +
      buildStudentAndStatusFilter(connection.get(), studentUserId,
                                  std::move(status)) +
      " ORDER BY r.created_at DESC, r.id DESC LIMIT " +
      std::to_string(pageSize) + " OFFSET " + std::to_string(offset);

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  std::vector<ReservationView> reservations;
  MYSQL_ROW row = nullptr;
  while ((row = mysql_fetch_row(result.get())) != nullptr) {
    reservations.push_back(mapReservationViewRow(row));
  }

  return reservations;
}

std::optional<ReservationView> ReservationDAO::getMyReservationById(
    std::uint64_t studentUserId,
    std::uint64_t reservationId) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT r.id, r.reservation_no, r.student_user_id, u.username, "
      "u.real_name, r.equipment_id, e.equipment_code, e.equipment_name, "
      "r.reservation_start_at, r.reservation_end_at, r.quantity, r.status, "
      "r.request_note, r.review_note, r.cancel_reason, r.reviewed_by, "
      "r.reviewed_at, r.created_at, r.updated_at "
      "FROM reservation r "
      "INNER JOIN `user` u ON u.id = r.student_user_id "
      "INNER JOIN equipment e ON e.id = r.equipment_id "
      "WHERE r.id = " +
      std::to_string(reservationId) + " AND r.student_user_id = " +
      std::to_string(studentUserId) + " LIMIT 1";

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MYSQL_ROW row = mysql_fetch_row(result.get());
  if (row == nullptr) {
    return std::nullopt;
  }

  return mapReservationViewRow(row);
}

std::uint64_t ReservationDAO::countAdminReservations(
    std::optional<std::string> status) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT COUNT(*) FROM reservation r" +
      buildStatusFilter(connection.get(), std::move(status));

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

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

std::vector<ReservationView> ReservationDAO::queryAdminReservationsPage(
    std::uint32_t pageNo,
    std::uint32_t pageSize,
    std::optional<std::string> status) const {
  const std::uint64_t offset =
      static_cast<std::uint64_t>(pageNo - 1) *
      static_cast<std::uint64_t>(pageSize);

  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT r.id, r.reservation_no, r.student_user_id, u.username, "
      "u.real_name, r.equipment_id, e.equipment_code, e.equipment_name, "
      "r.reservation_start_at, r.reservation_end_at, r.quantity, r.status, "
      "r.request_note, r.review_note, r.cancel_reason, r.reviewed_by, "
      "r.reviewed_at, r.created_at, r.updated_at "
      "FROM reservation r "
      "INNER JOIN `user` u ON u.id = r.student_user_id "
      "INNER JOIN equipment e ON e.id = r.equipment_id" +
      buildStatusFilter(connection.get(), std::move(status)) +
      " ORDER BY r.created_at DESC, r.id DESC LIMIT " +
      std::to_string(pageSize) + " OFFSET " + std::to_string(offset);

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  std::vector<ReservationView> reservations;
  MYSQL_ROW row = nullptr;
  while ((row = mysql_fetch_row(result.get())) != nullptr) {
    reservations.push_back(mapReservationViewRow(row));
  }

  return reservations;
}

std::optional<ReservationView> ReservationDAO::getReservationById(
    std::uint64_t reservationId) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT r.id, r.reservation_no, r.student_user_id, u.username, "
      "u.real_name, r.equipment_id, e.equipment_code, e.equipment_name, "
      "r.reservation_start_at, r.reservation_end_at, r.quantity, r.status, "
      "r.request_note, r.review_note, r.cancel_reason, r.reviewed_by, "
      "r.reviewed_at, r.created_at, r.updated_at "
      "FROM reservation r "
      "INNER JOIN `user` u ON u.id = r.student_user_id "
      "INNER JOIN equipment e ON e.id = r.equipment_id "
      "WHERE r.id = " + std::to_string(reservationId) + " LIMIT 1";

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MYSQL_ROW row = mysql_fetch_row(result.get());
  if (row == nullptr) {
    return std::nullopt;
  }

  return mapReservationViewRow(row);
}

std::uint32_t ReservationDAO::sumReservedQuantityForOverlap(
    std::uint64_t equipmentId,
    const std::string& reservationStartAt,
    const std::string& reservationEndAt) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "SELECT COALESCE(SUM(quantity), 0) "
      "FROM reservation "
      "WHERE equipment_id = " +
      std::to_string(equipmentId) +
      " AND status IN ('PENDING', 'APPROVED', 'BORROWED')"
      " AND reservation_start_at < '" +
      escapeSqlString(connection.get(), reservationEndAt) +
      "' AND reservation_end_at > '" +
      escapeSqlString(connection.get(), reservationStartAt) + "'";

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MYSQL_ROW row = mysql_fetch_row(result.get());
  if (row == nullptr) {
    return 0U;
  }

  return toUInt32(row[0]);
}

bool ReservationDAO::cancelMyReservation(std::uint64_t studentUserId,
                                         std::uint64_t reservationId,
                                         const std::string& cancelReason) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "UPDATE reservation "
      "SET status = 'CANCELED', cancel_reason = " +
      toNullableSqlLiteral(connection.get(), cancelReason) +
      " WHERE id = " + std::to_string(reservationId) +
      " AND student_user_id = " + std::to_string(studentUserId) +
      " AND status IN ('PENDING', 'APPROVED')";

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  return mysql_affected_rows(connection.get()) > 0;
}

bool ReservationDAO::reviewReservation(std::uint64_t reservationId,
                                       std::uint64_t reviewedBy,
                                       const std::string& targetStatus,
                                       const std::string& reviewNote) const {
  auto connection = createConnection(m_config);
  const std::string sql =
      "UPDATE reservation "
      "SET status = '" + escapeSqlString(connection.get(), targetStatus) +
      "', review_note = " +
      toNullableSqlLiteral(connection.get(), reviewNote) +
      ", reviewed_by = " + std::to_string(reviewedBy) +
      ", reviewed_at = NOW() "
      "WHERE id = " + std::to_string(reservationId) +
      " AND status = 'PENDING'";

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  return mysql_affected_rows(connection.get()) > 0;
}
