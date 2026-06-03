#ifndef EQUIPMENT_CATEGORY_SERVICE_HPP
#define EQUIPMENT_CATEGORY_SERVICE_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/EquipmentCategory.hpp"

#include <vector>

class EquipmentCategoryService {
public:
  explicit EquipmentCategoryService(DatabaseConfig databaseConfig);

  std::vector<EquipmentCategory> listAll() const;

private:
  DatabaseConfig m_databaseConfig;
};

#endif
