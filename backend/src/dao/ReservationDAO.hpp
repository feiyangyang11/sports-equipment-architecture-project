#ifndef RESERVATION_DAO_HPP
#define RESERVATION_DAO_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/Reservation.hpp"
#include "model/ReservationView.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class ReservationDAO {
public:
  explicit ReservationDAO(DatabaseConfig config);

  std::uint64_t create(const Reservation& reservation) const;
  std::uint64_t countMyReservations(
      std::uint64_t studentUserId,
      std::optional<std::string> status) const;
  std::vector<ReservationView> queryMyReservationsPage(
      std::uint64_t studentUserId,
      std::uint32_t pageNo,
      std::uint32_t pageSize,
      std::optional<std::string> status) const;
  std::optional<ReservationView> getMyReservationById(
      std::uint64_t studentUserId,
      std::uint64_t reservationId) const;
  std::uint64_t countAdminReservations(
      std::optional<std::string> status) const;
  std::vector<ReservationView> queryAdminReservationsPage(
      std::uint32_t pageNo,
      std::uint32_t pageSize,
      std::optional<std::string> status) const;
  std::optional<ReservationView> getReservationById(
      std::uint64_t reservationId) const;
  std::uint32_t sumReservedQuantityForOverlap(
      std::uint64_t equipmentId,
      const std::string& reservationStartAt,
      const std::string& reservationEndAt) const;
  bool cancelMyReservation(std::uint64_t studentUserId,
                           std::uint64_t reservationId,
                           const std::string& cancelReason) const;
  bool reviewReservation(std::uint64_t reservationId,
                         std::uint64_t reviewedBy,
                         const std::string& targetStatus,
                         const std::string& reviewNote) const;

private:
  DatabaseConfig m_config;
};

#endif
