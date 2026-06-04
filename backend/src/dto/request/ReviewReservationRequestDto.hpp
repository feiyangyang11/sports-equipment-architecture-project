#ifndef REVIEW_RESERVATION_REQUEST_DTO_HPP
#define REVIEW_RESERVATION_REQUEST_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class ReviewReservationRequestDto : public oatpp::DTO {
  DTO_INIT(ReviewReservationRequestDto, DTO)

  DTO_FIELD(String, reviewNote);
};

#include OATPP_CODEGEN_END(DTO)

#endif
