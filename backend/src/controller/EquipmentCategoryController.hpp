#ifndef EQUIPMENT_CATEGORY_CONTROLLER_HPP
#define EQUIPMENT_CATEGORY_CONTROLLER_HPP

#include "dto/response/EquipmentCategoryDto.hpp"
#include "dto/response/EquipmentCategoryListDto.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "service/EquipmentCategoryService.hpp"

#include <memory>
#include <utility>

#include OATPP_CODEGEN_BEGIN(ApiController)

class EquipmentCategoryController
    : public oatpp::web::server::api::ApiController {
public:
  static std::shared_ptr<EquipmentCategoryController> createShared(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<EquipmentCategoryService> equipmentCategoryService) {
    return std::make_shared<EquipmentCategoryController>(
        objectMapper, std::move(equipmentCategoryService));
  }

public:
  EquipmentCategoryController(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<EquipmentCategoryService> equipmentCategoryService)
      : oatpp::web::server::api::ApiController(objectMapper),
        m_equipmentCategoryService(std::move(equipmentCategoryService)) {}

public:
  ENDPOINT("GET", "/api/equipment-categories", listAll) {
    const auto categories = m_equipmentCategoryService->listAll();

    auto responseDto = EquipmentCategoryListDto::createShared();
    responseDto->items =
        oatpp::List<oatpp::Object<EquipmentCategoryDto>>::createShared();

    for (const auto& category : categories) {
      auto itemDto = EquipmentCategoryDto::createShared();
      itemDto->id = category.id;
      itemDto->categoryCode = category.categoryCode.c_str();
      itemDto->categoryName = category.categoryName.c_str();
      itemDto->status = category.status.c_str();
      responseDto->items->push_back(itemDto);
    }

    return createDtoResponse(Status::CODE_200, responseDto);
  }

private:
  std::shared_ptr<EquipmentCategoryService> m_equipmentCategoryService;
};

#include OATPP_CODEGEN_END(ApiController)

#endif
