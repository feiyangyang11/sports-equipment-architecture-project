#ifndef EQUIPMENT_CATEGORY_LIST_DTO_HPP
#define EQUIPMENT_CATEGORY_LIST_DTO_HPP

#include "dto/response/EquipmentCategoryDto.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class EquipmentCategoryListDto : public oatpp::DTO {
  DTO_INIT(EquipmentCategoryListDto, DTO)

  DTO_FIELD(List<Object<EquipmentCategoryDto>>, items);
};

#include OATPP_CODEGEN_END(DTO)

#endif
