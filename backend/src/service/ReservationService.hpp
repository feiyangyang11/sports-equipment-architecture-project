#ifndef RESERVATION_SERVICE_HPP
#define RESERVATION_SERVICE_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/ReservationPageResult.hpp"
#include "model/ReservationView.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

class ReservationServiceError : public std::runtime_error {
public:
  ReservationServiceError(int statusCode, std::string message);

  int getStatusCode() const noexcept;

private:
  int m_statusCode;
};

class ReservationService {
public:
  explicit ReservationService(DatabaseConfig databaseConfig);

  ReservationView createReservation(std::uint64_t studentUserId,
                                    std::uint64_t equipmentId,
                                    const std::string& reservationStartAt,
                                    const std::string& reservationEndAt,
                                    std::uint32_t quantity,
                                    const std::string& requestNote) const;
  ReservationPageResult queryMyPage(
      std::uint64_t studentUserId,
      std::uint32_t pageNo,
      std::uint32_t pageSize,
      std::optional<std::string> status) const;
  ReservationPageResult queryAdminPage(
      std::uint32_t pageNo,
      std::uint32_t pageSize,
      std::optional<std::string> status) const;
  std::optional<ReservationView> getMyById(std::uint64_t studentUserId,
                                           std::uint64_t reservationId) const;
  std::optional<ReservationView> getById(std::uint64_t reservationId) const;
  std::optional<ReservationView> cancelMyReservation(
      std::uint64_t studentUserId,
      std::uint64_t reservationId,
      const std::string& cancelReason) const;
  std::optional<ReservationView> approveReservation(
      std::uint64_t adminUserId,
      std::uint64_t reservationId,
      const std::string& reviewNote) const;
  std::optional<ReservationView> rejectReservation(
      std::uint64_t adminUserId,
      std::uint64_t reservationId,
      const std::string& reviewNote) const;

private:
  DatabaseConfig m_databaseConfig;
};

#endif
