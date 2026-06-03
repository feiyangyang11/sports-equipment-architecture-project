#include "service/EquipmentCategoryService.hpp"

#include "dao/EquipmentCategoryDAO.hpp"

#include <utility>

EquipmentCategoryService::EquipmentCategoryService(
    DatabaseConfig databaseConfig)
    : m_databaseConfig(std::move(databaseConfig)) {}

std::vector<EquipmentCategory> EquipmentCategoryService::listAll() const {
  EquipmentCategoryDAO equipmentCategoryDAO(m_databaseConfig);
  return equipmentCategoryDAO.listAll();
}
