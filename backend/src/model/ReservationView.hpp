#ifndef RESERVATION_VIEW_HPP
#define RESERVATION_VIEW_HPP

#include <cstdint>
#include <string>

class ReservationView {
public:
  std::uint64_t id{};
  std::string reservationNo;
  std::uint64_t studentUserId{};
  std::string studentUsername;
  std::string studentRealName;
  std::uint64_t equipmentId{};
  std::string equipmentCode;
  std::string equipmentName;
  std::string reservationStartAt;
  std::string reservationEndAt;
  std::uint32_t quantity{};
  std::string status;
  std::string requestNote;
  std::string reviewNote;
  std::string cancelReason;
  std::uint64_t reviewedBy{};
  std::string reviewedAt;
  std::string createdAt;
  std::string updatedAt;
};

#endif
