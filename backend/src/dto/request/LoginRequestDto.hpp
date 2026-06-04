#ifndef LOGIN_REQUEST_DTO_HPP
#define LOGIN_REQUEST_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class LoginRequestDto : public oatpp::DTO {
  DTO_INIT(LoginRequestDto, DTO)

  DTO_FIELD(String, username);
  DTO_FIELD(String, password);
};

#include OATPP_CODEGEN_END(DTO)

#endif
