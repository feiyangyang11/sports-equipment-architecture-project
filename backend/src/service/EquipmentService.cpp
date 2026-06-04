#include "service/EquipmentService.hpp"

#include "dao/EquipmentDAO.hpp"

#include <utility>

EquipmentService::EquipmentService(DatabaseConfig databaseConfig)
    : m_databaseConfig(std::move(databaseConfig)) {}

EquipmentPageResult EquipmentService::queryPage(std::uint32_t pageNo,
                                                std::uint32_t pageSize,
                                                std::optional<std::uint64_t> categoryId) const {
  EquipmentDAO equipmentDAO(m_databaseConfig);
  EquipmentPageResult result;
  result.pageNo = pageNo;
  result.pageSize = pageSize;
  result.total = equipmentDAO.countAll(categoryId);
  result.items = equipmentDAO.queryPage(pageNo, pageSize, categoryId);
  return result;
}

std::optional<Equipment> EquipmentService::getById(std::uint64_t id) const {
  EquipmentDAO equipmentDAO(m_databaseConfig);
  return equipmentDAO.getById(id);
}
