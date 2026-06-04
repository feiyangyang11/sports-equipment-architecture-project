#include "dao/UserDAO.hpp"

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

User mapUserRow(MYSQL_ROW row) {
  User user;
  user.id = toUInt64(row[0]);
  user.username = toString(row[1]);
  user.passwordHash = toString(row[2]);
  user.realName = toString(row[3]);
  user.role = toString(row[4]);
  user.studentNo = toString(row[5]);
  user.phone = toString(row[6]);
  user.email = toString(row[7]);
  user.status = toString(row[8]);
  user.lastLoginAt = toString(row[9]);
  user.createdAt = toString(row[10]);
  user.updatedAt = toString(row[11]);
  return user;
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

}  // namespace

UserDAO::UserDAO(DatabaseConfig config) : m_config(std::move(config)) {}

std::optional<User> UserDAO::findByCredentials(
    const std::string& username, const std::string& password) const {
  auto connection = createConnection(m_config);

  const auto escapedUsername = escapeSqlString(connection.get(), username);
  const auto escapedPassword = escapeSqlString(connection.get(), password);

  const std::string sql =
      "SELECT id, username, password_hash, real_name, role, student_no, "
      "phone, email, status, last_login_at, created_at, updated_at "
      "FROM `user` "
      "WHERE username = '" +
      escapedUsername + "' "
                        "AND password_hash = SHA2('" +
      escapedPassword + "', 256) "
                        "LIMIT 1";

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

  return mapUserRow(row);
}

std::optional<User> UserDAO::findById(std::uint64_t id) const {
  const std::string sql =
      "SELECT id, username, password_hash, real_name, role, student_no, "
      "phone, email, status, last_login_at, created_at, updated_at "
      "FROM `user` "
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

  return mapUserRow(row);
}

void UserDAO::updateLastLoginAt(std::uint64_t id) const {
  const std::string sql =
      "UPDATE `user` SET last_login_at = NOW() WHERE id = " +
      std::to_string(id);

  auto connection = createConnection(m_config);
  if (mysql_query(connection.get(), sql.c_str()) != 0) {
    throw std::runtime_error(mysql_error(connection.get()));
  }
}
