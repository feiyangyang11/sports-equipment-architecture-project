#ifndef BORROW_RECORD_HPP
#define BORROW_RECORD_HPP

#include <cstdint>
#include <string>

class BorrowRecord {
public:
  std::uint64_t id{};
  std::string borrowNo;
  std::uint64_t reservationId{};
  std::uint64_t studentUserId{};
  std::uint64_t equipmentId{};
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
