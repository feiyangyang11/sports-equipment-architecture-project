#ifndef EQUIPMENT_PAGE_DTO_HPP
#define EQUIPMENT_PAGE_DTO_HPP

#include "dto/response/EquipmentDto.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class EquipmentPageDto : public oatpp::DTO {
  DTO_INIT(EquipmentPageDto, DTO)

  DTO_FIELD(UInt32, pageNo);
  DTO_FIELD(UInt32, pageSize);
  DTO_FIELD(UInt64, total);
  DTO_FIELD(List<Object<EquipmentDto>>, items);
};

#include OATPP_CODEGEN_END(DTO)

#endif
