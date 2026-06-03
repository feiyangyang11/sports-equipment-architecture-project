#ifndef DATABASE_CONFIG_HPP
#define DATABASE_CONFIG_HPP

#include <array>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>

class DatabaseConfig {
public:
  std::string host;
  unsigned int port{3306};
  std::string database;
  std::string username;
  std::string password;
  std::string charset{"utf8mb4"};

public:
  static DatabaseConfig load() {
    static const std::array<const char*, 3> kCandidatePaths = {
        "config/database.conf",
        "../config/database.conf",
        "../../config/database.conf",
    };

    for (const char* path : kCandidatePaths) {
      std::ifstream input(path);
      if (!input.is_open()) {
        continue;
      }

      return loadFromStream(input);
    }

    throw std::runtime_error(
        "database.conf not found. Expected config/database.conf near the "
        "backend project or build directory.");
  }

private:
  static DatabaseConfig loadFromStream(std::istream& input) {
    DatabaseConfig config;

    std::string line;
    while (std::getline(input, line)) {
      line = trim(line);
      if (line.empty() || line[0] == '#') {
        continue;
      }

      const std::size_t separator = line.find('=');
      if (separator == std::string::npos) {
        continue;
      }

      const std::string key = trim(line.substr(0, separator));
      const std::string value = trim(line.substr(separator + 1));

      if (key == "host") {
        config.host = value;
      } else if (key == "port") {
        config.port = parsePort(value, 3306);
      } else if (key == "database") {
        config.database = value;
      } else if (key == "username") {
        config.username = value;
      } else if (key == "password") {
        config.password = value;
      } else if (key == "charset") {
        config.charset = value;
      }
    }

    if (config.host.empty()) {
      config.host = "127.0.0.1";
    }
    if (config.database.empty()) {
      config.database = "sports_equipment_management";
    }
    if (config.username.empty()) {
      config.username = "root";
    }
    if (config.charset.empty()) {
      config.charset = "utf8mb4";
    }

    return config;
  }

private:
  static std::string trim(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
      ++begin;
    }

    std::size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
      --end;
    }

    return value.substr(begin, end - begin);
  }

  static unsigned int parsePort(const std::string& value,
                                unsigned int fallback) {
    if (value.empty()) {
      return fallback;
    }

    try {
      return static_cast<unsigned int>(std::stoul(value));
    } catch (...) {
      return fallback;
    }
  }
};

#endif
