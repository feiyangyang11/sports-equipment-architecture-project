#ifndef EQUIPMENT_CATEGORY_DTO_HPP
#define EQUIPMENT_CATEGORY_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class EquipmentCategoryDto : public oatpp::DTO {
  DTO_INIT(EquipmentCategoryDto, DTO)

  DTO_FIELD(UInt64, id);
  DTO_FIELD(String, categoryCode);
  DTO_FIELD(String, categoryName);
  DTO_FIELD(String, status);
};

#include OATPP_CODEGEN_END(DTO)

#endif
