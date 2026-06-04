#ifndef BORROW_RECORD_DAO_HPP
#define BORROW_RECORD_DAO_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/BorrowRecord.hpp"
#include "model/BorrowRecordView.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class BorrowRecordDAO {
public:
  explicit BorrowRecordDAO(DatabaseConfig config);

  std::optional<std::uint64_t> createFromApprovedReservation(
      std::uint64_t reservationId,
      std::uint64_t borrowedBy,
      const std::string& borrowedAt,
      const std::string& dueAt) const;
  std::optional<std::uint64_t> returnBorrowRecord(std::uint64_t borrowRecordId,
                                                  std::uint64_t receivedBy,
                                                  const std::string& returnedAt,
                                                  const std::string& returnNote) const;
  std::uint64_t countMyBorrows(std::uint64_t studentUserId,
                               std::optional<std::string> status) const;
  std::vector<BorrowRecordView> queryMyBorrowsPage(
      std::uint64_t studentUserId,
      std::uint32_t pageNo,
      std::uint32_t pageSize,
      std::optional<std::string> status) const;
  std::optional<BorrowRecordView> getMyBorrowById(
      std::uint64_t studentUserId,
      std::uint64_t borrowRecordId) const;
  std::uint64_t countAdminBorrows(std::optional<std::string> status) const;
  std::vector<BorrowRecordView> queryAdminBorrowsPage(
      std::uint32_t pageNo,
      std::uint32_t pageSize,
      std::optional<std::string> status) const;
  std::optional<BorrowRecordView> getBorrowById(
      std::uint64_t borrowRecordId) const;

private:
  DatabaseConfig m_config;
};

#endif
