#ifndef USER_HPP
#define USER_HPP

#include <cstdint>
#include <string>

class User {
public:
  std::uint64_t id{};
  std::string username;
  std::string passwordHash;
  std::string realName;
  std::string role;
  std::string studentNo;
  std::string phone;
  std::string email;
  std::string status;
  std::string lastLoginAt;
  std::string createdAt;
  std::string updatedAt;
};

#endif
