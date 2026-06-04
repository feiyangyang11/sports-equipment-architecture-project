#ifndef EQUIPMENT_DAO_HPP
#define EQUIPMENT_DAO_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/Equipment.hpp"

#include <optional>
#include <vector>

class EquipmentDAO {
public:
  explicit EquipmentDAO(DatabaseConfig config);

  std::vector<Equipment> queryPage(std::uint32_t pageNo,
                                   std::uint32_t pageSize,
                                   std::optional<std::uint64_t> categoryId) const;
  std::uint64_t countAll(std::optional<std::uint64_t> categoryId) const;
  std::optional<Equipment> getById(std::uint64_t id) const;

private:
  DatabaseConfig m_config;
};

#endif
