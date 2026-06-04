#include "app/AppComponent.hpp"
#include "controller/AuthController.hpp"
#include "controller/BorrowRecordController.hpp"
#include "controller/EquipmentController.hpp"
#include "controller/EquipmentCategoryController.hpp"
#include "controller/HealthController.hpp"
#include "controller/ReservationController.hpp"
#include "dao/DatabaseConfig.hpp"
#include "oatpp/core/base/Environment.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/network/Server.hpp"
#include "service/BorrowRecordService.hpp"
#include "service/EquipmentService.hpp"
#include "service/EquipmentCategoryService.hpp"
#include "service/ReservationService.hpp"
#include "service/UserService.hpp"

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
  auto borrowRecordService =
      std::make_shared<BorrowRecordService>(databaseConfig);
  auto equipmentService = std::make_shared<EquipmentService>(databaseConfig);
  auto reservationService =
      std::make_shared<ReservationService>(databaseConfig);
  auto userService = std::make_shared<UserService>(databaseConfig);

  router->addController(HealthController::createShared(objectMapper));
  router->addController(EquipmentCategoryController::createShared(
      objectMapper, equipmentCategoryService));
  router->addController(BorrowRecordController::createShared(
      objectMapper, borrowRecordService, userService));
  router->addController(
      EquipmentController::createShared(objectMapper, equipmentService));
  router->addController(ReservationController::createShared(
      objectMapper, reservationService, userService));
  router->addController(AuthController::createShared(objectMapper, userService));

  oatpp::network::Server server(connectionProvider, connectionHandler);

  std::cout << "sports_equipment_backend listening on http://localhost:8000"
            << std::endl;
  server.run();

  oatpp::base::Environment::destroy();
  return 0;
}
