#ifndef EQUIPMENT_SERVICE_HPP
#define EQUIPMENT_SERVICE_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/Equipment.hpp"
#include "model/EquipmentPageResult.hpp"

#include <optional>
#include <vector>

class EquipmentService {
public:
  explicit EquipmentService(DatabaseConfig databaseConfig);

  EquipmentPageResult queryPage(std::uint32_t pageNo,
                                std::uint32_t pageSize,
                                std::optional<std::uint64_t> categoryId) const;
  std::optional<Equipment> getById(std::uint64_t id) const;

private:
  DatabaseConfig m_databaseConfig;
};

#endif
