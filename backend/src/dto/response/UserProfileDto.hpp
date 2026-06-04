#ifndef USER_PROFILE_DTO_HPP
#define USER_PROFILE_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class UserProfileDto : public oatpp::DTO {
  DTO_INIT(UserProfileDto, DTO)

  DTO_FIELD(UInt64, id);
  DTO_FIELD(String, username);
  DTO_FIELD(String, realName);
  DTO_FIELD(String, role);
  DTO_FIELD(String, studentNo);
  DTO_FIELD(String, status);
};

#include OATPP_CODEGEN_END(DTO)

#endif
