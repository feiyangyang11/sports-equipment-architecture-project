#ifndef LOGIN_RESPONSE_DTO_HPP
#define LOGIN_RESPONSE_DTO_HPP

#include "dto/response/UserProfileDto.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class LoginResponseDto : public oatpp::DTO {
  DTO_INIT(LoginResponseDto, DTO)

  DTO_FIELD(String, token);
  DTO_FIELD(Object<UserProfileDto>, user);
};

#include OATPP_CODEGEN_END(DTO)

#endif
