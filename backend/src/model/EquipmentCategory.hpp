#ifndef EQUIPMENT_CATEGORY_HPP
#define EQUIPMENT_CATEGORY_HPP

#include <cstdint>
#include <string>

class EquipmentCategory {
public:
  std::uint64_t id{};
  std::string categoryCode;
  std::string categoryName;
  std::string description;
  std::int32_t sortOrder{};
  std::string status;
  std::string createdAt;
  std::string updatedAt;
};

#endif
