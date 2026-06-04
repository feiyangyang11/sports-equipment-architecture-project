#ifndef RESERVATION_CONTROLLER_HPP
#define RESERVATION_CONTROLLER_HPP

#include "dto/request/CancelReservationRequestDto.hpp"
#include "dto/request/CreateReservationRequestDto.hpp"
#include "dto/request/ReviewReservationRequestDto.hpp"
#include "dto/response/ReservationDto.hpp"
#include "dto/response/ReservationPageDto.hpp"
#include "model/User.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/protocol/http/Http.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "service/ReservationService.hpp"
#include "service/UserService.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include OATPP_CODEGEN_BEGIN(ApiController)

class ReservationController : public oatpp::web::server::api::ApiController {
public:
  static std::shared_ptr<ReservationController> createShared(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<ReservationService> reservationService,
      std::shared_ptr<UserService> userService) {
    return std::make_shared<ReservationController>(
        objectMapper, std::move(reservationService), std::move(userService));
  }

public:
  ReservationController(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper,
      std::shared_ptr<ReservationService> reservationService,
      std::shared_ptr<UserService> userService)
      : oatpp::web::server::api::ApiController(objectMapper),
        m_reservationService(std::move(reservationService)),
        m_userService(std::move(userService)) {}

public:
  ENDPOINT("POST", "/api/reservations", createReservation,
           HEADER(String, authorization, "Authorization"),
           BODY_DTO(Object<CreateReservationRequestDto>, requestDto)) {
    User currentUser;
    if (const auto authError =
            requireActiveStudent(authorization, currentUser)) {
      return authError;
    }

    const auto equipmentId =
        requestDto && requestDto->equipmentId ? requestDto->equipmentId.getValue(0)
                                              : 0ULL;
    const auto quantity =
        requestDto && requestDto->quantity ? requestDto->quantity.getValue(0)
                                           : 0U;
    const std::string reservationStartAt =
        requestDto && requestDto->reservationStartAt != nullptr
            ? requestDto->reservationStartAt->c_str()
            : "";
    const std::string reservationEndAt =
        requestDto && requestDto->reservationEndAt != nullptr
            ? requestDto->reservationEndAt->c_str()
            : "";
    const std::string requestNote =
        requestDto && requestDto->requestNote != nullptr
            ? requestDto->requestNote->c_str()
            : "";

    try {
      const auto reservation = m_reservationService->createReservation(
          currentUser.id, equipmentId, reservationStartAt, reservationEndAt,
          quantity, requestNote);
      return createDtoResponse(Status::CODE_201, toDto(reservation));
    } catch (const ReservationServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

  ENDPOINT("GET", "/api/reservations/my", queryMyReservations,
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
      const auto pageResult = m_reservationService->queryMyPage(
          currentUser.id, pageNoValue, pageSizeValue, statusFilter);

      auto responseDto = ReservationPageDto::createShared();
      responseDto->pageNo = pageResult.pageNo;
      responseDto->pageSize = pageResult.pageSize;
      responseDto->total = pageResult.total;
      responseDto->items =
          oatpp::List<oatpp::Object<ReservationDto>>::createShared();

      for (const auto& reservation : pageResult.items) {
        responseDto->items->push_back(toDto(reservation));
      }

      return createDtoResponse(Status::CODE_200, responseDto);
    } catch (const ReservationServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

  ENDPOINT("GET", "/api/reservations/my/{id}", getMyReservationById,
           HEADER(String, authorization, "Authorization"), PATH(UInt64, id)) {
    User currentUser;
    if (const auto authError =
            requireActiveStudent(authorization, currentUser)) {
      return authError;
    }

    const auto reservationId = id.getValue(0);
    if (reservationId == 0) {
      return createResponse(Status::CODE_400, "id must be >= 1");
    }

    const auto reservation =
        m_reservationService->getMyById(currentUser.id, reservationId);
    if (!reservation.has_value()) {
      return createResponse(Status::CODE_404, "reservation not found");
    }

    return createDtoResponse(Status::CODE_200, toDto(*reservation));
  }

  ENDPOINT("POST", "/api/reservations/my/{id}/cancel", cancelMyReservation,
           HEADER(String, authorization, "Authorization"), PATH(UInt64, id),
           BODY_DTO(Object<CancelReservationRequestDto>, requestDto)) {
    User currentUser;
    if (const auto authError =
            requireActiveStudent(authorization, currentUser)) {
      return authError;
    }

    const auto reservationId = id.getValue(0);
    if (reservationId == 0) {
      return createResponse(Status::CODE_400, "id must be >= 1");
    }

    const std::string cancelReason =
        requestDto && requestDto->cancelReason != nullptr
            ? requestDto->cancelReason->c_str()
            : "";

    try {
      const auto reservation = m_reservationService->cancelMyReservation(
          currentUser.id, reservationId, cancelReason);
      if (!reservation.has_value()) {
        return createResponse(Status::CODE_404, "reservation not found");
      }

      return createDtoResponse(Status::CODE_200, toDto(*reservation));
    } catch (const ReservationServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

  ENDPOINT("GET", "/api/admin/reservations", queryAdminReservations,
           HEADER(String, authorization, "Authorization"),
           QUERY(UInt32, pageNo, "pageNo", 1),
           QUERY(UInt32, pageSize, "pageSize", 10),
           QUERY(String, status, "status", nullptr)) {
    User currentUser;
    if (const auto authError =
            requireActiveAdmin(authorization, currentUser)) {
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
      const auto pageResult = m_reservationService->queryAdminPage(
          pageNoValue, pageSizeValue, statusFilter);

      auto responseDto = ReservationPageDto::createShared();
      responseDto->pageNo = pageResult.pageNo;
      responseDto->pageSize = pageResult.pageSize;
      responseDto->total = pageResult.total;
      responseDto->items =
          oatpp::List<oatpp::Object<ReservationDto>>::createShared();

      for (const auto& reservation : pageResult.items) {
        responseDto->items->push_back(toDto(reservation));
      }

      return createDtoResponse(Status::CODE_200, responseDto);
    } catch (const ReservationServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

  ENDPOINT("POST", "/api/admin/reservations/{id}/approve", approveReservation,
           HEADER(String, authorization, "Authorization"), PATH(UInt64, id),
           BODY_DTO(Object<ReviewReservationRequestDto>, requestDto)) {
    User currentUser;
    if (const auto authError =
            requireActiveAdmin(authorization, currentUser)) {
      return authError;
    }

    const auto reservationId = id.getValue(0);
    if (reservationId == 0) {
      return createResponse(Status::CODE_400, "id must be >= 1");
    }

    const std::string reviewNote =
        requestDto && requestDto->reviewNote != nullptr
            ? requestDto->reviewNote->c_str()
            : "";

    try {
      const auto reservation = m_reservationService->approveReservation(
          currentUser.id, reservationId, reviewNote);
      if (!reservation.has_value()) {
        return createResponse(Status::CODE_404, "reservation not found");
      }

      return createDtoResponse(Status::CODE_200, toDto(*reservation));
    } catch (const ReservationServiceError& error) {
      return createResponse(statusFromError(error), error.what());
    }
  }

  ENDPOINT("POST", "/api/admin/reservations/{id}/reject", rejectReservation,
           HEADER(String, authorization, "Authorization"), PATH(UInt64, id),
           BODY_DTO(Object<ReviewReservationRequestDto>, requestDto)) {
    User currentUser;
    if (const auto authError =
            requireActiveAdmin(authorization, currentUser)) {
      return authError;
    }

    const auto reservationId = id.getValue(0);
    if (reservationId == 0) {
      return createResponse(Status::CODE_400, "id must be >= 1");
    }

    const std::string reviewNote =
        requestDto && requestDto->reviewNote != nullptr
            ? requestDto->reviewNote->c_str()
            : "";

    try {
      const auto reservation = m_reservationService->rejectReservation(
          currentUser.id, reservationId, reviewNote);
      if (!reservation.has_value()) {
        return createResponse(Status::CODE_404, "reservation not found");
      }

      return createDtoResponse(Status::CODE_200, toDto(*reservation));
    } catch (const ReservationServiceError& error) {
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

  static Status statusFromError(const ReservationServiceError& error) {
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

  static oatpp::Object<ReservationDto> toDto(
      const ReservationView& reservation) {
    auto itemDto = ReservationDto::createShared();
    itemDto->id = reservation.id;
    itemDto->reservationNo = reservation.reservationNo.c_str();
    itemDto->studentUserId = reservation.studentUserId;
    itemDto->studentUsername = reservation.studentUsername.c_str();
    itemDto->studentRealName = reservation.studentRealName.c_str();
    itemDto->equipmentId = reservation.equipmentId;
    itemDto->equipmentCode = reservation.equipmentCode.c_str();
    itemDto->equipmentName = reservation.equipmentName.c_str();
    itemDto->reservationStartAt = reservation.reservationStartAt.c_str();
    itemDto->reservationEndAt = reservation.reservationEndAt.c_str();
    itemDto->quantity = reservation.quantity;
    itemDto->status = reservation.status.c_str();
    itemDto->requestNote = reservation.requestNote.c_str();
    itemDto->reviewNote = reservation.reviewNote.c_str();
    itemDto->cancelReason = reservation.cancelReason.c_str();
    itemDto->reviewedAt = reservation.reviewedAt.c_str();
    itemDto->createdAt = reservation.createdAt.c_str();
    itemDto->updatedAt = reservation.updatedAt.c_str();
    return itemDto;
  }

private:
  std::shared_ptr<ReservationService> m_reservationService;
  std::shared_ptr<UserService> m_userService;
};

#include OATPP_CODEGEN_END(ApiController)

#endif
