#ifndef BORROW_RECORD_CONTROLLER_HPP
#define BORROW_RECORD_CONTROLLER_HPP

#include "dto/request/CreateBorrowRequestDto.hpp"
#include "dto/request/ReturnBorrowRequestDto.hpp"
#include "dto/response/BorrowRecordDto.hpp"
#include "dto/response/BorrowRecordPageDto.hpp"
#include "model/User.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/protocol/http/Http.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "service/BorrowRecordService.hpp"
#include "service/UserService.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include OATPP_CODEGEN_BEGIN(ApiController)

class BorrowRecordController : public oatpp::web::server::api::ApiController {
public:
  static std::shared_ptr<BorrowRecordController> createShared(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<BorrowRecordService> borrowRecordService,
      std::shared_ptr<UserService> userService) {
    return std::make_shared<BorrowRecordController>(
        objectMapper, std::move(borrowRecordService), std::move(userService));
  }

public:
  BorrowRecordController(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<BorrowRecordService> borrowRecordService,
      std::shared_ptr<UserService> userService)
      : oatpp::web::server::api::ApiController(objectMapper),
        m_borrowRecordService(std::move(borrowRecordService)),
        m_userService(std::move(userService)) {}

public:
  ENDPOINT("POST", "/api/admin/borrows", createBorrowRecord,
           HEADER(String, authorization, "Authorization"),
           BODY_DTO(Object<CreateBorrowRequestDto>, requestDto)) {
    User currentUser;
    if (const auto authError = requireActiveAdmin(authorization, currentUser)) {
      return authError;
    }

    const auto reservationId =
        requestDto && requestDto->reservationId
            ? requestDto->reservationId.getValue(0)
            : 0ULL;
    const std::string borrowedAt =
        requestDto && requestDto->borrowedAt != nullptr
            ? requestDto->borrowedAt->c_str()
            : "";
    const std::string dueAt =
        requestDto && requestDto->dueAt != nullptr
            ? requestDto->dueAt->c_str()
            : "";

    try {
      const auto borrowRecord = m_borrowRecordService->createBorrowRecord(
          currentUser.id, reservationId, borrowedAt, dueAt);
      return createDtoResponse(Status::CODE_201, toDto(borrowRecord));
    } catch (const BorrowRecordServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

  ENDPOINT("GET", "/api/borrows/my", queryMyBorrows,
           HEADER(String, authorization, "Authorization"),
           QUERY(UInt32, pageNo, "pageNo", 1),
           QUERY(UInt32, pageSize, "pageSize", 10),
           QUERY(String, status, "status", nullptr)) {
    User currentUser;
    if (const auto authError =
            requireActiveStudent(authorization, currentUser)) {
      return authError;
    }

    const auto pageNoValue = pageNo.getValue(1);
    const auto pageSizeValue = pageSize.getValue(10);
    if (pageNoValue == 0 || pageSizeValue == 0 || pageSizeValue > 100) {
      return createResponse(Status::CODE_400,
                            "pageNo must be >= 1, pageSize must be in 1..100");
    }

    const std::optional<std::string> statusFilter =
        status != nullptr && !std::string(status->c_str()).empty()
            ? std::optional<std::string>(status->c_str())
            : std::nullopt;

    try {
      const auto pageResult = m_borrowRecordService->queryMyPage(
          currentUser.id, pageNoValue, pageSizeValue, statusFilter);

      auto responseDto = BorrowRecordPageDto::createShared();
      responseDto->pageNo = pageResult.pageNo;
      responseDto->pageSize = pageResult.pageSize;
      responseDto->total = pageResult.total;
      responseDto->items =
          oatpp::List<oatpp::Object<BorrowRecordDto>>::createShared();

      for (const auto& borrowRecord : pageResult.items) {
        responseDto->items->push_back(toDto(borrowRecord));
      }

      return createDtoResponse(Status::CODE_200, responseDto);
    } catch (const BorrowRecordServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

  ENDPOINT("GET", "/api/borrows/my/{id}", getMyBorrowById,
           HEADER(String, authorization, "Authorization"), PATH(UInt64, id)) {
    User currentUser;
    if (const auto authError =
            requireActiveStudent(authorization, currentUser)) {
      return authError;
    }

    const auto borrowRecordId = id.getValue(0);
    if (borrowRecordId == 0) {
      return createResponse(Status::CODE_400, "id must be >= 1");
    }

    const auto borrowRecord =
        m_borrowRecordService->getMyById(currentUser.id, borrowRecordId);
    if (!borrowRecord.has_value()) {
      return createResponse(Status::CODE_404, "borrow record not found");
    }

    return createDtoResponse(Status::CODE_200, toDto(*borrowRecord));
  }

  ENDPOINT("GET", "/api/admin/borrows", queryAdminBorrows,
           HEADER(String, authorization, "Authorization"),
           QUERY(UInt32, pageNo, "pageNo", 1),
           QUERY(UInt32, pageSize, "pageSize", 10),
           QUERY(String, status, "status", nullptr)) {
    User currentUser;
    if (const auto authError = requireActiveAdmin(authorization, currentUser)) {
      return authError;
    }

    const auto pageNoValue = pageNo.getValue(1);
    const auto pageSizeValue = pageSize.getValue(10);
    if (pageNoValue == 0 || pageSizeValue == 0 || pageSizeValue > 100) {
      return createResponse(Status::CODE_400,
                            "pageNo must be >= 1, pageSize must be in 1..100");
    }

    const std::optional<std::string> statusFilter =
        status != nullptr && !std::string(status->c_str()).empty()
            ? std::optional<std::string>(status->c_str())
            : std::nullopt;

    try {
      const auto pageResult = m_borrowRecordService->queryAdminPage(
          pageNoValue, pageSizeValue, statusFilter);

      auto responseDto = BorrowRecordPageDto::createShared();
      responseDto->pageNo = pageResult.pageNo;
      responseDto->pageSize = pageResult.pageSize;
      responseDto->total = pageResult.total;
      responseDto->items =
          oatpp::List<oatpp::Object<BorrowRecordDto>>::createShared();

      for (const auto& borrowRecord : pageResult.items) {
        responseDto->items->push_back(toDto(borrowRecord));
      }

      return createDtoResponse(Status::CODE_200, responseDto);
    } catch (const BorrowRecordServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

  ENDPOINT("GET", "/api/admin/borrows/{id}", getBorrowById,
           HEADER(String, authorization, "Authorization"), PATH(UInt64, id)) {
    User currentUser;
    if (const auto authError = requireActiveAdmin(authorization, currentUser)) {
      return authError;
    }

    const auto borrowRecordId = id.getValue(0);
    if (borrowRecordId == 0) {
      return createResponse(Status::CODE_400, "id must be >= 1");
    }

    const auto borrowRecord = m_borrowRecordService->getById(borrowRecordId);
    if (!borrowRecord.has_value()) {
      return createResponse(Status::CODE_404, "borrow record not found");
    }

    return createDtoResponse(Status::CODE_200, toDto(*borrowRecord));
  }

  ENDPOINT("POST", "/api/admin/borrows/{id}/return", returnBorrowRecord,
           HEADER(String, authorization, "Authorization"), PATH(UInt64, id),
           BODY_DTO(Object<ReturnBorrowRequestDto>, requestDto)) {
    User currentUser;
    if (const auto authError = requireActiveAdmin(authorization, currentUser)) {
      return authError;
    }

    const auto borrowRecordId = id.getValue(0);
    if (borrowRecordId == 0) {
      return createResponse(Status::CODE_400, "id must be >= 1");
    }

    const std::string returnedAt =
        requestDto && requestDto->returnedAt != nullptr
            ? requestDto->returnedAt->c_str()
            : "";
    const std::string returnNote =
        requestDto && requestDto->returnNote != nullptr
            ? requestDto->returnNote->c_str()
            : "";

    try {
      const auto borrowRecord = m_borrowRecordService->returnBorrowRecord(
          currentUser.id, borrowRecordId, returnedAt, returnNote);
      if (!borrowRecord.has_value()) {
        return createResponse(Status::CODE_404, "borrow record not found");
      }

      return createDtoResponse(Status::CODE_200, toDto(*borrowRecord));
    } catch (const BorrowRecordServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

private:
  std::shared_ptr<OutgoingResponse> requireActiveStudent(
      const oatpp::String& authorization, User& currentUser) {
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

    if (user->role != "STUDENT") {
      return createResponse(Status::CODE_403, "student role is required");
    }

    currentUser = *user;
    return nullptr;
  }

  std::shared_ptr<OutgoingResponse> requireActiveAdmin(
      const oatpp::String& authorization, User& currentUser) {
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

    if (user->role != "ADMIN") {
      return createResponse(Status::CODE_403, "admin role is required");
    }

    currentUser = *user;
    return nullptr;
  }

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

  static Status statusFromError(const BorrowRecordServiceError& error) {
    switch (error.getStatusCode()) {
      case 400:
        return Status::CODE_400;
      case 401:
        return Status::CODE_401;
      case 403:
        return Status::CODE_403;
      case 404:
        return Status::CODE_404;
      case 409:
        return Status::CODE_409;
      default:
        return Status::CODE_500;
    }
  }

  static oatpp::Object<BorrowRecordDto> toDto(
      const BorrowRecordView& borrowRecord) {
    auto itemDto = BorrowRecordDto::createShared();
    itemDto->id = borrowRecord.id;
    itemDto->borrowNo = borrowRecord.borrowNo.c_str();
    itemDto->reservationId = borrowRecord.reservationId;
    itemDto->reservationNo = borrowRecord.reservationNo.c_str();
    itemDto->studentUserId = borrowRecord.studentUserId;
    itemDto->studentUsername = borrowRecord.studentUsername.c_str();
    itemDto->studentRealName = borrowRecord.studentRealName.c_str();
    itemDto->equipmentId = borrowRecord.equipmentId;
    itemDto->equipmentCode = borrowRecord.equipmentCode.c_str();
    itemDto->equipmentName = borrowRecord.equipmentName.c_str();
    itemDto->quantity = borrowRecord.quantity;
    itemDto->borrowedBy = borrowRecord.borrowedBy;
    itemDto->borrowedAt = borrowRecord.borrowedAt.c_str();
    itemDto->dueAt = borrowRecord.dueAt.c_str();
    itemDto->receivedBy = borrowRecord.receivedBy;
    itemDto->returnedAt = borrowRecord.returnedAt.c_str();
    itemDto->status = borrowRecord.status.c_str();
    itemDto->returnNote = borrowRecord.returnNote.c_str();
    itemDto->createdAt = borrowRecord.createdAt.c_str();
    itemDto->updatedAt = borrowRecord.updatedAt.c_str();
    return itemDto;
  }

private:
  std::shared_ptr<BorrowRecordService> m_borrowRecordService;
  std::shared_ptr<UserService> m_userService;
};

#include OATPP_CODEGEN_END(ApiController)

#endif
