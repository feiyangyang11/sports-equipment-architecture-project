#include "dao/EquipmentCategoryDAO.hpp"

#include <mysql/mysql.h>

#include <cstdlib>
#include <memory>
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

std::int32_t toInt32(const char* value) {
  return value != nullptr ? static_cast<std::int32_t>(std::strtol(value, nullptr, 10))
                          : 0;
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

}  // namespace

EquipmentCategoryDAO::EquipmentCategoryDAO(DatabaseConfig config)
    : m_config(std::move(config)) {}

std::vector<EquipmentCategory> EquipmentCategoryDAO::listAll() const {
  static const char* kSql =
      "SELECT id, category_code, category_name, description, sort_order, "
      "status, created_at, updated_at "
      "FROM equipment_category "
      "ORDER BY sort_order, id";

  auto connection = createConnection(m_config);

  if (mysql_query(connection.get(), kSql) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  MysqlResult result(mysql_store_result(connection.get()), mysql_free_result);
  if (result == nullptr) {
    throw std::runtime_error(mysql_error(connection.get()));
  }

  std::vector<EquipmentCategory> categories;
  MYSQL_ROW row = nullptr;

  while ((row = mysql_fetch_row(result.get())) != nullptr) {
    EquipmentCategory category;
    category.id = toUInt64(row[0]);
    category.categoryCode = toString(row[1]);
    category.categoryName = toString(row[2]);
    category.description = toString(row[3]);
    category.sortOrder = toInt32(row[4]);
    category.status = toString(row[5]);
    category.createdAt = toString(row[6]);
    category.updatedAt = toString(row[7]);
    categories.push_back(std::move(category));
  }

  return categories;
}
