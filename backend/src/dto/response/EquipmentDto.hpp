#ifndef EQUIPMENT_DTO_HPP
#define EQUIPMENT_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class EquipmentDto : public oatpp::DTO {
  DTO_INIT(EquipmentDto, DTO)

  DTO_FIELD(UInt64, id);
  DTO_FIELD(UInt64, categoryId);
  DTO_FIELD(String, equipmentCode);
  DTO_FIELD(String, equipmentName);
  DTO_FIELD(String, specification);
  DTO_FIELD(String, storageLocation);
  DTO_FIELD(UInt32, totalStock);
  DTO_FIELD(UInt32, availableStock);
  DTO_FIELD(String, status);
};

#include OATPP_CODEGEN_END(DTO)

#endif
