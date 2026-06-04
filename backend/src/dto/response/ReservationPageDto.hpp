#ifndef RESERVATION_PAGE_DTO_HPP
#define RESERVATION_PAGE_DTO_HPP

#include "dto/response/ReservationDto.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class ReservationPageDto : public oatpp::DTO {
  DTO_INIT(ReservationPageDto, DTO)

  DTO_FIELD(UInt32, pageNo);
  DTO_FIELD(UInt32, pageSize);
  DTO_FIELD(UInt64, total);
  DTO_FIELD(List<Object<ReservationDto>>, items);
};

#include OATPP_CODEGEN_END(DTO)

#endif
