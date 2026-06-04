#ifndef RESERVATION_DTO_HPP
#define RESERVATION_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class ReservationDto : public oatpp::DTO {
  DTO_INIT(ReservationDto, DTO)

  DTO_FIELD(UInt64, id);
  DTO_FIELD(String, reservationNo);
  DTO_FIELD(UInt64, studentUserId);
  DTO_FIELD(String, studentUsername);
  DTO_FIELD(String, studentRealName);
  DTO_FIELD(UInt64, equipmentId);
  DTO_FIELD(String, equipmentCode);
  DTO_FIELD(String, equipmentName);
  DTO_FIELD(String, reservationStartAt);
  DTO_FIELD(String, reservationEndAt);
  DTO_FIELD(UInt32, quantity);
  DTO_FIELD(String, status);
  DTO_FIELD(String, requestNote);
  DTO_FIELD(String, reviewNote);
  DTO_FIELD(String, cancelReason);
  DTO_FIELD(String, reviewedAt);
  DTO_FIELD(String, createdAt);
  DTO_FIELD(String, updatedAt);
};

#include OATPP_CODEGEN_END(DTO)

#endif
