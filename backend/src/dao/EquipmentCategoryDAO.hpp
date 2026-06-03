#ifndef EQUIPMENT_CATEGORY_DAO_HPP
#define EQUIPMENT_CATEGORY_DAO_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/EquipmentCategory.hpp"

#include <vector>

class EquipmentCategoryDAO {
public:
  explicit EquipmentCategoryDAO(DatabaseConfig config);

  std::vector<EquipmentCategory> listAll() const;

private:
  DatabaseConfig m_config;
};

#endif
