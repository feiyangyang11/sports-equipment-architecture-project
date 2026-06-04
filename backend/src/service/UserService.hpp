#ifndef USER_SERVICE_HPP
#define USER_SERVICE_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/User.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class UserService {
public:
  explicit UserService(DatabaseConfig databaseConfig);

  std::optional<User> authenticate(const std::string& username,
                                   const std::string& password) const;
  std::string issueToken(std::uint64_t userId);
  std::optional<User> getCurrentUser(const std::string& token) const;
  bool revokeToken(const std::string& token);

private:
  DatabaseConfig m_databaseConfig;
  mutable std::mutex m_sessionMutex;
  std::unordered_map<std::string, std::uint64_t> m_tokenToUserId;
};

#endif
