#ifndef HEALTH_CONTROLLER_HPP
#define HEALTH_CONTROLLER_HPP

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/protocol/http/Http.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

class HealthController : public oatpp::web::server::api::ApiController {
public:
  static std::shared_ptr<HealthController> createShared(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper) {
    return std::make_shared<HealthController>(objectMapper);
  }

public:
  explicit HealthController(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper)
      : oatpp::web::server::api::ApiController(objectMapper) {}

public:
  ENDPOINT("GET", "/health", health) {
    auto response = createResponse(Status::CODE_200, "{\"status\":\"ok\"}");
    response->putHeader(oatpp::web::protocol::http::Header::CONTENT_TYPE,
                        "application/json");
    return response;
  }
};

#include OATPP_CODEGEN_END(ApiController)

#endif
