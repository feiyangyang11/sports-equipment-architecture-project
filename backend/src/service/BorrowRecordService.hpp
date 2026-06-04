#ifndef BORROW_RECORD_SERVICE_HPP
#define BORROW_RECORD_SERVICE_HPP

#include "dao/DatabaseConfig.hpp"
#include "model/BorrowRecordPageResult.hpp"
#include "model/BorrowRecordView.hpp"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

class BorrowRecordServiceError : public std::runtime_error {
public:
  BorrowRecordServiceError(int statusCode, std::string message);

  int getStatusCode() const noexcept;

private:
  int m_statusCode;
};

class BorrowRecordService {
public:
  explicit BorrowRecordService(DatabaseConfig databaseConfig);

  BorrowRecordView createBorrowRecord(std::uint64_t adminUserId,
                                      std::uint64_t reservationId,
                                      const std::string& borrowedAt,
                                      const std::string& dueAt) const;
  BorrowRecordPageResult queryMyPage(
      std::uint64_t studentUserId,
      std::uint32_t pageNo,
      std::uint32_t pageSize,
      std::optional<std::string> status) const;
  BorrowRecordPageResult queryAdminPage(
      std::uint32_t pageNo,
      std::uint32_t pageSize,
      std::optional<std::string> status) const;
  std::optional<BorrowRecordView> getMyById(std::uint64_t studentUserId,
                                            std::uint64_t borrowRecordId) const;
  std::optional<BorrowRecordView> getById(std::uint64_t borrowRecordId) const;
  std::optional<BorrowRecordView> returnBorrowRecord(
      std::uint64_t adminUserId,
      std::uint64_t borrowRecordId,
      const std::string& returnedAt,
      const std::string& returnNote) const;

private:
  DatabaseConfig m_databaseConfig;
};

#endif
