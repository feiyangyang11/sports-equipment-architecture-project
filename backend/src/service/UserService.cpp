#include "service/UserService.hpp"

#include "dao/UserDAO.hpp"

#include <iomanip>
#include <random>
#include <sstream>
#include <utility>

namespace {

std::string generateToken() {
  static thread_local std::mt19937_64 generator(std::random_device{}());
  std::uniform_int_distribution<std::uint64_t> distribution;

  std::ostringstream output;
  output << std::hex << std::setfill('0');
  output << std::setw(16) << distribution(generator);
  output << std::setw(16) << distribution(generator);
  return output.str();
}

}  // namespace

UserService::UserService(DatabaseConfig databaseConfig)
    : m_databaseConfig(std::move(databaseConfig)) {}

std::optional<User> UserService::authenticate(const std::string& username,
                                              const std::string& password) const {
  if (username.empty() || password.empty()) {
    return std::nullopt;
  }

  UserDAO userDAO(m_databaseConfig);
  return userDAO.findByCredentials(username, password);
}

std::string UserService::issueToken(std::uint64_t userId) {
  UserDAO userDAO(m_databaseConfig);
  userDAO.updateLastLoginAt(userId);

  const auto token = generateToken();
  std::lock_guard<std::mutex> guard(m_sessionMutex);
  m_tokenToUserId[token] = userId;
  return token;
}

std::optional<User> UserService::getCurrentUser(
    const std::string& token) const {
  if (token.empty()) {
    return std::nullopt;
  }

  std::uint64_t userId = 0;
  {
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    const auto iterator = m_tokenToUserId.find(token);
    if (iterator == m_tokenToUserId.end()) {
      return std::nullopt;
    }
    userId = iterator->second;
  }

  UserDAO userDAO(m_databaseConfig);
  return userDAO.findById(userId);
}

bool UserService::revokeToken(const std::string& token) {
  if (token.empty()) {
    return false;
  }

  std::lock_guard<std::mutex> guard(m_sessionMutex);
  return m_tokenToUserId.erase(token) > 0;
}
