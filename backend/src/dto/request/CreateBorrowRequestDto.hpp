#ifndef CREATE_BORROW_REQUEST_DTO_HPP
#define CREATE_BORROW_REQUEST_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class CreateBorrowRequestDto : public oatpp::DTO {
  DTO_INIT(CreateBorrowRequestDto, DTO)

  DTO_FIELD(UInt64, reservationId);
  DTO_FIELD(String, borrowedAt);
  DTO_FIELD(String, dueAt);
};

#include OATPP_CODEGEN_END(DTO)

#endif
