#ifndef BORROW_RECORD_VIEW_HPP
#define BORROW_RECORD_VIEW_HPP

#include <cstdint>
#include <string>

class BorrowRecordView {
public:
  std::uint64_t id{};
  std::string borrowNo;
  std::uint64_t reservationId{};
  std::string reservationNo;
  std::uint64_t studentUserId{};
  std::string studentUsername;
  std::string studentRealName;
  std::uint64_t equipmentId{};
  std::string equipmentCode;
  std::string equipmentName;
  std::uint32_t quantity{};
  std::uint64_t borrowedBy{};
  std::string borrowedAt;
  std::string dueAt;
  std::uint64_t receivedBy{};
  std::string returnedAt;
  std::string status;
  std::string returnNote;
  std::string createdAt;
  std::string updatedAt;
};

#endif
