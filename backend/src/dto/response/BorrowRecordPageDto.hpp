#ifndef BORROW_RECORD_PAGE_DTO_HPP
#define BORROW_RECORD_PAGE_DTO_HPP

#include "dto/response/BorrowRecordDto.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class BorrowRecordPageDto : public oatpp::DTO {
  DTO_INIT(BorrowRecordPageDto, DTO)

  DTO_FIELD(UInt32, pageNo);
  DTO_FIELD(UInt32, pageSize);
  DTO_FIELD(UInt64, total);
  DTO_FIELD(List<Object<BorrowRecordDto>>, items);
};

#include OATPP_CODEGEN_END(DTO)

#endif
