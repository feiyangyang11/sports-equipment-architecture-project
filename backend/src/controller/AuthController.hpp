#ifndef AUTH_CONTROLLER_HPP
#define AUTH_CONTROLLER_HPP

#include "dto/request/LoginRequestDto.hpp"
#include "dto/response/LoginResponseDto.hpp"
#include "dto/response/UserProfileDto.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/protocol/http/Http.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "service/UserService.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include OATPP_CODEGEN_BEGIN(ApiController)

class AuthController : public oatpp::web::server::api::ApiController {
public:
  static std::shared_ptr<AuthController> createShared(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<UserService> userService) {
    return std::make_shared<AuthController>(objectMapper,
                                            std::move(userService));
  }

public:
  AuthController(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<UserService> userService)
      : oatpp::web::server::api::ApiController(objectMapper),
        m_userService(std::move(userService)) {}

public:
  ENDPOINT("POST", "/api/login", login,
           BODY_DTO(Object<LoginRequestDto>, loginRequestDto)) {
    const std::string username =
        loginRequestDto && loginRequestDto->username != nullptr
            ? loginRequestDto->username->c_str()
            : "";
    const std::string password =
        loginRequestDto && loginRequestDto->password != nullptr
            ? loginRequestDto->password->c_str()
            : "";

    if (username.empty() || password.empty()) {
      return createResponse(Status::CODE_400,
                            "username and password are required");
    }

    const auto user = m_userService->authenticate(username, password);
    if (!user.has_value()) {
      return createResponse(Status::CODE_401, "invalid username or password");
    }

    if (user->status != "ACTIVE") {
      return createResponse(Status::CODE_403, "user is disabled");
    }

    const auto token = m_userService->issueToken(user->id);

    auto responseDto = LoginResponseDto::createShared();
    responseDto->token = token.c_str();
    responseDto->user = toProfileDto(*user);

    return createDtoResponse(Status::CODE_200, responseDto);
  }

  ENDPOINT("GET", "/api/me", getMe,
           HEADER(String, authorization, "Authorization")) {
    const auto token = parseBearerToken(authorization);
    if (!token.has_value()) {
      return createResponse(Status::CODE_401,
                            "Authorization header with Bearer token is required");
    }

    const auto user = m_userService->getCurrentUser(*token);
    if (!user.has_value()) {
      return createResponse(Status::CODE_401, "invalid or expired token");
    }

    if (user->status != "ACTIVE") {
      return createResponse(Status::CODE_403, "user is disabled");
    }

    return createDtoResponse(Status::CODE_200, toProfileDto(*user));
  }

  ENDPOINT("POST", "/api/logout", logout,
           HEADER(String, authorization, "Authorization")) {
    const auto token = parseBearerToken(authorization);
    if (!token.has_value()) {
      return createResponse(Status::CODE_401,
                            "Authorization header with Bearer token is required");
    }

    if (!m_userService->revokeToken(*token)) {
      return createResponse(Status::CODE_401, "invalid or expired token");
    }

    auto response = createResponse(Status::CODE_200, "{\"message\":\"logout success\"}");
    response->putHeader(oatpp::web::protocol::http::Header::CONTENT_TYPE,
                        "application/json");
    return response;
  }

private:
  static std::optional<std::string> parseBearerToken(
      const oatpp::String& authorization) {
    const std::string authorizationValue =
        authorization != nullptr ? authorization->c_str() : "";
    static const std::string kPrefix = "Bearer ";

    if (authorizationValue.rfind(kPrefix, 0) != 0) {
      return std::nullopt;
    }

    const std::string token = authorizationValue.substr(kPrefix.size());
    if (token.empty()) {
      return std::nullopt;
    }

    return token;
  }

  static oatpp::Object<UserProfileDto> toProfileDto(const User& user) {
    auto profileDto = UserProfileDto::createShared();
    profileDto->id = user.id;
    profileDto->username = user.username.c_str();
    profileDto->realName = user.realName.c_str();
    profileDto->role = user.role.c_str();
    profileDto->studentNo = user.studentNo.c_str();
    profileDto->status = user.status.c_str();
    return profileDto;
  }

private:
  std::shared_ptr<UserService> m_userService;
};

#include OATPP_CODEGEN_END(ApiController)

#endif
