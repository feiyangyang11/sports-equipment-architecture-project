#ifndef EQUIPMENT_HPP
#define EQUIPMENT_HPP

#include <cstdint>
#include <string>

class Equipment {
public:
  std::uint64_t id{};
  std::uint64_t categoryId{};
  std::string equipmentCode;
  std::string equipmentName;
  std::string specification;
  std::string storageLocation;
  std::string description;
  std::uint32_t totalStock{};
  std::uint32_t availableStock{};
  std::string status;
  std::string createdAt;
  std::string updatedAt;
};

#endif
