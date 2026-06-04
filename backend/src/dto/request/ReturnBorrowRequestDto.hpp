#ifndef RETURN_BORROW_REQUEST_DTO_HPP
#define RETURN_BORROW_REQUEST_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class ReturnBorrowRequestDto : public oatpp::DTO {
  DTO_INIT(ReturnBorrowRequestDto, DTO)

  DTO_FIELD(String, returnedAt);
  DTO_FIELD(String, returnNote);
};

#include OATPP_CODEGEN_END(DTO)

#endif
