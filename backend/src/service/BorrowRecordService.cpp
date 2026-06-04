#include "service/BorrowRecordService.hpp"

#include "dao/BorrowRecordDAO.hpp"
#include "dao/ReservationDAO.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

namespace {

bool isAllowedBorrowStatus(const std::string& status) {
  static const std::array<std::string, 4> kAllowedStatuses = {
      "BORROWING", "RETURNED", "OVERDUE", "CLOSED"};
  return std::find(kAllowedStatuses.begin(), kAllowedStatuses.end(), status) !=
         kAllowedStatuses.end();
}

std::string toUpper(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::toupper(character));
                 });
  return value;
}

bool tryParseDateTime(const std::string& value, std::tm& result) {
  result = {};
  std::istringstream input(value);
  input >> std::get_time(&result, "%Y-%m-%d %H:%M:%S");
  if (input.fail()) {
    return false;
  }

  char trailingCharacter = '\0';
  if (input >> trailingCharacter) {
    return false;
  }

  std::ostringstream normalized;
  normalized << std::put_time(&result, "%Y-%m-%d %H:%M:%S");
  return normalized.str() == value;
}

bool isValidDateTime(const std::string& value) {
  std::tm parsedTime = {};
  return tryParseDateTime(value, parsedTime);
}

bool isLaterThan(const std::string& leftTime, const std::string& rightTime) {
  std::tm left = {};
  std::tm right = {};
  if (!tryParseDateTime(leftTime, left) || !tryParseDateTime(rightTime, right)) {
    return false;
  }

  return std::mktime(&left) > std::mktime(&right);
}

}  // namespace

BorrowRecordServiceError::BorrowRecordServiceError(int statusCode,
                                                   std::string message)
    : std::runtime_error(std::move(message)), m_statusCode(statusCode) {}

int BorrowRecordServiceError::getStatusCode() const noexcept {
  return m_statusCode;
}

BorrowRecordService::BorrowRecordService(DatabaseConfig databaseConfig)
    : m_databaseConfig(std::move(databaseConfig)) {}

BorrowRecordView BorrowRecordService::createBorrowRecord(
    std::uint64_t adminUserId, std::uint64_t reservationId,
    const std::string& borrowedAt, const std::string& dueAt) const {
  if (adminUserId == 0 || reservationId == 0) {
    throw BorrowRecordServiceError(400,
                                   "adminUserId and reservationId must be >= 1");
  }

  if (!isValidDateTime(borrowedAt) || !isValidDateTime(dueAt)) {
    throw BorrowRecordServiceError(
        400, "borrowedAt and dueAt must use YYYY-MM-DD HH:MM:SS");
  }

  if (!isLaterThan(dueAt, borrowedAt)) {
    throw BorrowRecordServiceError(400, "dueAt must be later than borrowedAt");
  }

  ReservationDAO reservationDAO(m_databaseConfig);
  const auto reservation = reservationDAO.getReservationById(reservationId);
  if (!reservation.has_value()) {
    throw BorrowRecordServiceError(404, "reservation not found");
  }

  if (reservation->status != "APPROVED") {
    throw BorrowRecordServiceError(
        409, "only APPROVED reservations can be borrowed");
  }

  BorrowRecordDAO borrowRecordDAO(m_databaseConfig);
  const auto borrowRecordId = borrowRecordDAO.createFromApprovedReservation(
      reservationId, adminUserId, borrowedAt, dueAt);
  if (!borrowRecordId.has_value()) {
    throw BorrowRecordServiceError(
        409, "borrow create failed because reservation status or stock changed");
  }

  const auto createdBorrowRecord =
      borrowRecordDAO.getBorrowById(*borrowRecordId);
  if (!createdBorrowRecord.has_value()) {
    throw std::runtime_error("failed to load created borrow record");
  }

  return *createdBorrowRecord;
}

BorrowRecordPageResult BorrowRecordService::queryMyPage(
    std::uint64_t studentUserId, std::uint32_t pageNo, std::uint32_t pageSize,
    std::optional<std::string> status) const {
  std::optional<std::string> normalizedStatus = std::nullopt;
  if (status.has_value() && !status->empty()) {
    normalizedStatus = toUpper(*status);
    if (!isAllowedBorrowStatus(*normalizedStatus)) {
      throw BorrowRecordServiceError(400, "status filter is invalid");
    }
  }

  BorrowRecordDAO borrowRecordDAO(m_databaseConfig);
  BorrowRecordPageResult result;
  result.pageNo = pageNo;
  result.pageSize = pageSize;
  result.total = borrowRecordDAO.countMyBorrows(studentUserId, normalizedStatus);
  result.items = borrowRecordDAO.queryMyBorrowsPage(studentUserId, pageNo,
                                                    pageSize, normalizedStatus);
  return result;
}

BorrowRecordPageResult BorrowRecordService::queryAdminPage(
    std::uint32_t pageNo, std::uint32_t pageSize,
    std::optional<std::string> status) const {
  std::optional<std::string> normalizedStatus = std::nullopt;
  if (status.has_value() && !status->empty()) {
    normalizedStatus = toUpper(*status);
    if (!isAllowedBorrowStatus(*normalizedStatus)) {
      throw BorrowRecordServiceError(400, "status filter is invalid");
    }
  }

  BorrowRecordDAO borrowRecordDAO(m_databaseConfig);
  BorrowRecordPageResult result;
  result.pageNo = pageNo;
  result.pageSize = pageSize;
  result.total = borrowRecordDAO.countAdminBorrows(normalizedStatus);
  result.items =
      borrowRecordDAO.queryAdminBorrowsPage(pageNo, pageSize, normalizedStatus);
  return result;
}

std::optional<BorrowRecordView> BorrowRecordService::getMyById(
    std::uint64_t studentUserId, std::uint64_t borrowRecordId) const {
  BorrowRecordDAO borrowRecordDAO(m_databaseConfig);
  return borrowRecordDAO.getMyBorrowById(studentUserId, borrowRecordId);
}

std::optional<BorrowRecordView> BorrowRecordService::getById(
    std::uint64_t borrowRecordId) const {
  BorrowRecordDAO borrowRecordDAO(m_databaseConfig);
  return borrowRecordDAO.getBorrowById(borrowRecordId);
}

std::optional<BorrowRecordView> BorrowRecordService::returnBorrowRecord(
    std::uint64_t adminUserId, std::uint64_t borrowRecordId,
    const std::string& returnedAt, const std::string& returnNote) const {
  if (adminUserId == 0 || borrowRecordId == 0) {
    throw BorrowRecordServiceError(400,
                                   "adminUserId and borrowRecordId must be >= 1");
  }

  if (!isValidDateTime(returnedAt)) {
    throw BorrowRecordServiceError(
        400, "returnedAt must use YYYY-MM-DD HH:MM:SS");
  }

  if (returnNote.size() > 255) {
    throw BorrowRecordServiceError(400, "returnNote length must be <= 255");
  }

  BorrowRecordDAO borrowRecordDAO(m_databaseConfig);
  const auto borrowRecord = borrowRecordDAO.getBorrowById(borrowRecordId);
  if (!borrowRecord.has_value()) {
    return std::nullopt;
  }

  if (borrowRecord->status != "BORROWING") {
    throw BorrowRecordServiceError(
        409, "only BORROWING borrow records can be returned");
  }

  if (!isLaterThan(returnedAt, borrowRecord->borrowedAt) &&
      returnedAt != borrowRecord->borrowedAt) {
    throw BorrowRecordServiceError(
        400, "returnedAt must be later than or equal to borrowedAt");
  }

  const auto returnedBorrowRecord = borrowRecordDAO.returnBorrowRecord(
      borrowRecordId, adminUserId, returnedAt, returnNote);
  if (!returnedBorrowRecord.has_value()) {
    throw BorrowRecordServiceError(
        409, "borrow return failed because status changed");
  }

  return borrowRecordDAO.getBorrowById(*returnedBorrowRecord);
}
