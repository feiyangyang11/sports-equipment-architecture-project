#ifndef EQUIPMENT_PAGE_RESULT_HPP
#define EQUIPMENT_PAGE_RESULT_HPP

#include "model/Equipment.hpp"

#include <cstdint>
#include <vector>

class EquipmentPageResult {
public:
  std::uint32_t pageNo{};
  std::uint32_t pageSize{};
  std::uint64_t total{};
  std::vector<Equipment> items;
};

#endif
