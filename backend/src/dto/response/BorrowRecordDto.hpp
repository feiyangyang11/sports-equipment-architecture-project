#ifndef BORROW_RECORD_DTO_HPP
#define BORROW_RECORD_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class BorrowRecordDto : public oatpp::DTO {
  DTO_INIT(BorrowRecordDto, DTO)

  DTO_FIELD(UInt64, id);
  DTO_FIELD(String, borrowNo);
  DTO_FIELD(UInt64, reservationId);
  DTO_FIELD(String, reservationNo);
  DTO_FIELD(UInt64, studentUserId);
  DTO_FIELD(String, studentUsername);
  DTO_FIELD(String, studentRealName);
  DTO_FIELD(UInt64, equipmentId);
  DTO_FIELD(String, equipmentCode);
  DTO_FIELD(String, equipmentName);
  DTO_FIELD(UInt32, quantity);
  DTO_FIELD(UInt64, borrowedBy);
  DTO_FIELD(String, borrowedAt);
  DTO_FIELD(String, dueAt);
  DTO_FIELD(UInt64, receivedBy);
  DTO_FIELD(String, returnedAt);
  DTO_FIELD(String, status);
  DTO_FIELD(String, returnNote);
  DTO_FIELD(String, createdAt);
  DTO_FIELD(String, updatedAt);
};

#include OATPP_CODEGEN_END(DTO)

#endif
