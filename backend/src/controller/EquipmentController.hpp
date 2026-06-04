#ifndef EQUIPMENT_CONTROLLER_HPP
#define EQUIPMENT_CONTROLLER_HPP

#include "dto/response/EquipmentDto.hpp"
#include "dto/response/EquipmentPageDto.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/protocol/http/Http.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "service/EquipmentService.hpp"

#include <memory>
#include <optional>
#include <utility>

#include OATPP_CODEGEN_BEGIN(ApiController)

class EquipmentController : public oatpp::web::server::api::ApiController {
public:
  static std::shared_ptr<EquipmentController> createShared(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<EquipmentService> equipmentService) {
    return std::make_shared<EquipmentController>(objectMapper,
                                                 std::move(equipmentService));
  }

public:
  EquipmentController(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<EquipmentService> equipmentService)
      : oatpp::web::server::api::ApiController(objectMapper),
        m_equipmentService(std::move(equipmentService)) {}

public:
  ENDPOINT("GET", "/api/equipment", queryPage,
           QUERY(UInt32, pageNo, "pageNo", 1),
           QUERY(UInt32, pageSize, "pageSize", 10),
           QUERY(UInt64, categoryId, "categoryId", nullptr)) {
    const auto pageNoValue = pageNo.getValue(1);
    const auto pageSizeValue = pageSize.getValue(10);
    const auto categoryIdValue = categoryId.getValue(0);

    if (pageNoValue == 0 || pageSizeValue == 0 || pageSizeValue > 100) {
      return createResponse(Status::CODE_400,
                            "pageNo must be >= 1, pageSize must be in 1..100");
    }

    const std::optional<std::uint64_t> categoryIdFilter =
        categoryIdValue == 0 ? std::nullopt
                             : std::optional<std::uint64_t>(categoryIdValue);

    const auto pageResult = m_equipmentService->queryPage(
        pageNoValue, pageSizeValue, categoryIdFilter);

    auto responseDto = EquipmentPageDto::createShared();
    responseDto->pageNo = pageResult.pageNo;
    responseDto->pageSize = pageResult.pageSize;
    responseDto->total = pageResult.total;
    responseDto->items = oatpp::List<oatpp::Object<EquipmentDto>>::createShared();

    for (const auto& equipment : pageResult.items) {
      responseDto->items->push_back(toDto(equipment));
    }

    return createDtoResponse(Status::CODE_200, responseDto);
  }

  ENDPOINT("GET", "/api/equipment/{id}", getById, PATH(UInt64, id)) {
    const auto equipmentId = id.getValue(0);
    if (equipmentId == 0) {
      return createResponse(Status::CODE_400, "id must be >= 1");
    }

    const auto equipment = m_equipmentService->getById(equipmentId);
    if (!equipment.has_value()) {
      return createResponse(Status::CODE_404, "equipment not found");
    }

    return createDtoResponse(Status::CODE_200, toDto(equipment.value()));
  }

private:
  static oatpp::Object<EquipmentDto> toDto(const Equipment& equipment) {
    auto itemDto = EquipmentDto::createShared();
    itemDto->id = equipment.id;
    itemDto->categoryId = equipment.categoryId;
    itemDto->equipmentCode = equipment.equipmentCode.c_str();
    itemDto->equipmentName = equipment.equipmentName.c_str();
    itemDto->specification = equipment.specification.c_str();
    itemDto->storageLocation = equipment.storageLocation.c_str();
    itemDto->totalStock = equipment.totalStock;
    itemDto->availableStock = equipment.availableStock;
    itemDto->status = equipment.status.c_str();
    return itemDto;
  }

  std::shared_ptr<EquipmentService> m_equipmentService;
};

#include OATPP_CODEGEN_END(ApiController)

#endif
