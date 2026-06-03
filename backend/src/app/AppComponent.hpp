#ifndef APP_COMPONENT_HPP
#define APP_COMPONENT_HPP

#include "oatpp/core/macro/component.hpp"
#include "oatpp/network/ConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/web/server/HttpRouter.hpp"

class AppComponent {
public:
  // Provide the TCP listener used by the HTTP server.
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>,
                         serverConnectionProvider)
  ([] {
    return oatpp::network::tcp::server::ConnectionProvider::createShared(
        {"0.0.0.0", 8000, oatpp::network::Address::IP_4});
  }());

  // Store route registrations such as GET /health.
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>,
                         httpRouter)
  ([] {
    return oatpp::web::server::HttpRouter::createShared();
  }());

  // Convert HTTP requests into controller dispatches through the router.
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>,
                         serverConnectionHandler)
  ([] {
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
    return oatpp::web::server::HttpConnectionHandler::createShared(router);
  }());

  // Serialize controller responses to JSON.
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>,
                         apiObjectMapper)
  ([] {
    return oatpp::parser::json::mapping::ObjectMapper::createShared();
  }());
};

#endif
