#include "app/AppComponent.hpp"
#include "controller/EquipmentCategoryController.hpp"
#include "controller/HealthController.hpp"
#include "dao/DatabaseConfig.hpp"
#include "oatpp/core/base/Environment.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/network/Server.hpp"
#include "service/EquipmentCategoryService.hpp"

#include <iostream>
#include <memory>

int main() {
  oatpp::base::Environment::init();

  AppComponent appComponent;
  const auto databaseConfig = DatabaseConfig::load();

  OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
  OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>,
                  objectMapper);
  OATPP_COMPONENT(
      std::shared_ptr<oatpp::network::ServerConnectionProvider>,
      connectionProvider);
  OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>,
                  connectionHandler);

  auto equipmentCategoryService =
      std::make_shared<EquipmentCategoryService>(databaseConfig);

  router->addController(HealthController::createShared(objectMapper));
  router->addController(EquipmentCategoryController::createShared(
      objectMapper, equipmentCategoryService));

  oatpp::network::Server server(connectionProvider, connectionHandler);

  std::cout << "sports_equipment_backend listening on http://localhost:8000"
            << std::endl;
  server.run();

  oatpp::base::Environment::destroy();
  return 0;
}
