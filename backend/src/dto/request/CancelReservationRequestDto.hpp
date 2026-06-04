#ifndef CANCEL_RESERVATION_REQUEST_DTO_HPP
#define CANCEL_RESERVATION_REQUEST_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class CancelReservationRequestDto : public oatpp::DTO {
  DTO_INIT(CancelReservationRequestDto, DTO)

  DTO_FIELD(String, cancelReason);
};

#include OATPP_CODEGEN_END(DTO)

#endif
