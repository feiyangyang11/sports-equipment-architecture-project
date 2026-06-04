#ifndef CREATE_RESERVATION_REQUEST_DTO_HPP
#define CREATE_RESERVATION_REQUEST_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class CreateReservationRequestDto : public oatpp::DTO {
  DTO_INIT(CreateReservationRequestDto, DTO)

  DTO_FIELD(UInt64, equipmentId);
  DTO_FIELD(String, reservationStartAt);
  DTO_FIELD(String, reservationEndAt);
  DTO_FIELD(UInt32, quantity);
  DTO_FIELD(String, requestNote);
};

#include OATPP_CODEGEN_END(DTO)

#endif
