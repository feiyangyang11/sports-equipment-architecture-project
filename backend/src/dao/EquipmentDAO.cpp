#include "dao/EquipmentDAO.hpp"

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

Equipment mapEquipmentRow(MYSQL_ROW row) {
  Equipment equipment;
  equipment.id = toUInt64(row[0]);
  equipment.categoryId = toUInt64(row[1]);
  equipment.equipmentCode = toString(row[2]);
  equipment.equipmentName = toString(row[3]);
  equipment.specification = toString(row[4]);
  equipment.storageLocation = toString(row[5]);
  equipment.description = toString(row[6]);
  equipment.totalStock = toUInt32(row[7]);
  equipment.availableStock = toUInt32(row[8]);
  equipment.status = toString(row[9]);
  equipment.createdAt = toString(row[10]);
  equipment.updatedAt = toString(row[11]);
  return equipment;
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

std::string buildCategoryFilter(std::optional<std::uint64_t> categoryId) {
  if (!categoryId.has_value()) {
    return "";
  }
  return " WHERE category_id = " + std::to_string(categoryId.value()) + " ";
}

}  // namespace

EquipmentDAO::EquipmentDAO(DatabaseConfig config) : m_config(std::move(config)) {}

std::vector<Equipment> EquipmentDAO::queryPage(std::uint32_t pageNo,
                                               std::uint32_t pageSize,
                                               std::optional<std::uint64_t> categoryId) const {
  const std::uint64_t offset =
      static_cast<std::uint64_t>(pageNo - 1) * static_cast<std::uint64_t>(pageSize);
  const std::string sql =
      "SELECT id, category_id, equipment_code, equipment_name, specification, "
      "storage_location, description, total_stock, available_stock, status, "
      "created_at, updated_at "
      "FROM equipment " +
      buildCategoryFilter(categoryId) +
      "ORDER BY id "
      "LIMIT " +
      std::to_string(pageSize) + " OFFSET " + std::to_string(offset);

  auto connection = createConnection(m_config);

  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  std::vector<Equipment> equipments;
  MYSQL_ROW row = nullptr;

  while ((row = mysql_fetch_row(result.get())) != nullptr) {
    equipments.push_back(mapEquipmentRow(row));
  }

  return equipments;
}

std::uint64_t EquipmentDAO::countAll(
    std::optional<std::uint64_t> categoryId) const {
  const std::string sql =
      "SELECT COUNT(*) FROM equipment" + buildCategoryFilter(categoryId);

  auto connection = createConnection(m_config);

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

std::optional<Equipment> EquipmentDAO::getById(std::uint64_t id) const {
  const std::string sql =
      "SELECT id, category_id, equipment_code, equipment_name, specification, "
      "storage_location, description, total_stock, available_stock, status, "
      "created_at, updated_at "
      "FROM equipment "
      "WHERE id = " +
      std::to_string(id) + " LIMIT 1";

  auto connection = createConnection(m_config);

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

  return mapEquipmentRow(row);
}
