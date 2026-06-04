#ifndef USER_DAO_HPP
#define USER_DAO_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/User.hpp"

#include <optional>
#include <string>

class UserDAO {
public:
  explicit UserDAO(DatabaseConfig config);

  std::optional<User> findByCredentials(const std::string& username,
                                        const std::string& password) const;
  std::optional<User> findById(std::uint64_t id) const;
  void updateLastLoginAt(std::uint64_t id) const;

private:
  DatabaseConfig m_config;
};

#endif
