#include "service/ReservationService.hpp"

#include "dao/EquipmentDAO.hpp"
#include "dao/ReservationDAO.hpp"
#include "model/Reservation.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <utility>

namespace {

bool isAllowedReservationStatus(const std::string& status) {
  static const std::array<std::string, 6> kAllowedStatuses = {
      "PENDING", "APPROVED", "REJECTED", "CANCELED", "BORROWED",
      "COMPLETED"};
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

bool isEndAfterStart(const std::string& reservationStartAt,
                     const std::string& reservationEndAt) {
  std::tm startTime = {};
  std::tm endTime = {};
  if (!tryParseDateTime(reservationStartAt, startTime) ||
      !tryParseDateTime(reservationEndAt, endTime)) {
    return false;
  }

  return std::mktime(&endTime) > std::mktime(&startTime);
}

std::string generateReservationNo() {
  static thread_local std::mt19937 generator(std::random_device{}());
  std::uniform_int_distribution<int> distribution(0, 9999);

  const auto now = std::chrono::system_clock::now();
  const std::time_t timeValue = std::chrono::system_clock::to_time_t(now);

  std::tm localTime = {};
#ifdef _WIN32
  localtime_s(&localTime, &timeValue);
#else
  localtime_r(&timeValue, &localTime);
#endif

  std::ostringstream output;
  output << "RES";
  output << std::put_time(&localTime, "%Y%m%d%H%M%S");
  output << std::setw(4) << std::setfill('0') << distribution(generator);
  return output.str();
}

}  // namespace

ReservationServiceError::ReservationServiceError(int statusCode,
                                                 std::string message)
    : std::runtime_error(std::move(message)), m_statusCode(statusCode) {}

int ReservationServiceError::getStatusCode() const noexcept {
  return m_statusCode;
}

ReservationService::ReservationService(DatabaseConfig databaseConfig)
    : m_databaseConfig(std::move(databaseConfig)) {}

ReservationView ReservationService::createReservation(
    std::uint64_t studentUserId,
    std::uint64_t equipmentId,
    const std::string& reservationStartAt,
    const std::string& reservationEndAt,
    std::uint32_t quantity,
    const std::string& requestNote) const {
  if (studentUserId == 0 || equipmentId == 0) {
    throw ReservationServiceError(400,
                                  "studentUserId and equipmentId must be >= 1");
  }

  if (quantity == 0) {
    throw ReservationServiceError(400, "quantity must be >= 1");
  }

  if (!isValidDateTime(reservationStartAt) ||
      !isValidDateTime(reservationEndAt)) {
    throw ReservationServiceError(
        400, "reservationStartAt and reservationEndAt must use YYYY-MM-DD HH:MM:SS");
  }

  if (!isEndAfterStart(reservationStartAt, reservationEndAt)) {
    throw ReservationServiceError(
        400, "reservationEndAt must be later than reservationStartAt");
  }

  if (requestNote.size() > 255) {
    throw ReservationServiceError(400, "requestNote length must be <= 255");
  }

  EquipmentDAO equipmentDAO(m_databaseConfig);
  ReservationDAO reservationDAO(m_databaseConfig);

  const auto equipment = equipmentDAO.getById(equipmentId);
  if (!equipment.has_value()) {
    throw ReservationServiceError(404, "equipment not found");
  }

  if (equipment->status != "AVAILABLE") {
    throw ReservationServiceError(409, "equipment is not available");
  }

  if (quantity > equipment->availableStock) {
    throw ReservationServiceError(
        409, "requested quantity exceeds current available stock");
  }

  const auto reservedQuantity = reservationDAO.sumReservedQuantityForOverlap(
      equipmentId, reservationStartAt, reservationEndAt);
  if (reservedQuantity + quantity > equipment->availableStock) {
    throw ReservationServiceError(
        409,
        "requested quantity exceeds available stock in the selected time range");
  }

  Reservation reservation;
  reservation.reservationNo = generateReservationNo();
  reservation.studentUserId = studentUserId;
  reservation.equipmentId = equipmentId;
  reservation.reservationStartAt = reservationStartAt;
  reservation.reservationEndAt = reservationEndAt;
  reservation.quantity = quantity;
  reservation.status = "PENDING";
  reservation.requestNote = requestNote;

  const auto reservationId = reservationDAO.create(reservation);
  const auto createdReservation =
      reservationDAO.getMyReservationById(studentUserId, reservationId);
  if (!createdReservation.has_value()) {
    throw std::runtime_error("failed to load created reservation");
  }

  return *createdReservation;
}

ReservationPageResult ReservationService::queryMyPage(
    std::uint64_t studentUserId,
    std::uint32_t pageNo,
    std::uint32_t pageSize,
    std::optional<std::string> status) const {
  std::optional<std::string> normalizedStatus = std::nullopt;
  if (status.has_value() && !status->empty()) {
    normalizedStatus = toUpper(*status);
    if (!isAllowedReservationStatus(*normalizedStatus)) {
      throw ReservationServiceError(400, "status filter is invalid");
    }
  }

  ReservationDAO reservationDAO(m_databaseConfig);
  ReservationPageResult result;
  result.pageNo = pageNo;
  result.pageSize = pageSize;
  result.total =
      reservationDAO.countMyReservations(studentUserId, normalizedStatus);
  result.items = reservationDAO.queryMyReservationsPage(studentUserId, pageNo,
                                                        pageSize,
                                                        normalizedStatus);
  return result;
}

ReservationPageResult ReservationService::queryAdminPage(
    std::uint32_t pageNo,
    std::uint32_t pageSize,
    std::optional<std::string> status) const {
  std::optional<std::string> normalizedStatus = std::nullopt;
  if (status.has_value() && !status->empty()) {
    normalizedStatus = toUpper(*status);
    if (!isAllowedReservationStatus(*normalizedStatus)) {
      throw ReservationServiceError(400, "status filter is invalid");
    }
  }

  ReservationDAO reservationDAO(m_databaseConfig);
  ReservationPageResult result;
  result.pageNo = pageNo;
  result.pageSize = pageSize;
  result.total = reservationDAO.countAdminReservations(normalizedStatus);
  result.items =
      reservationDAO.queryAdminReservationsPage(pageNo, pageSize,
                                                normalizedStatus);
  return result;
}

std::optional<ReservationView> ReservationService::getMyById(
    std::uint64_t studentUserId,
    std::uint64_t reservationId) const {
  ReservationDAO reservationDAO(m_databaseConfig);
  return reservationDAO.getMyReservationById(studentUserId, reservationId);
}

std::optional<ReservationView> ReservationService::getById(
    std::uint64_t reservationId) const {
  ReservationDAO reservationDAO(m_databaseConfig);
  return reservationDAO.getReservationById(reservationId);
}

std::optional<ReservationView> ReservationService::cancelMyReservation(
    std::uint64_t studentUserId,
    std::uint64_t reservationId,
    const std::string& cancelReason) const {
  if (cancelReason.size() > 255) {
    throw ReservationServiceError(400, "cancelReason length must be <= 255");
  }

  ReservationDAO reservationDAO(m_databaseConfig);
  const auto currentReservation =
      reservationDAO.getMyReservationById(studentUserId, reservationId);
  if (!currentReservation.has_value()) {
    return std::nullopt;
  }

  if (currentReservation->status != "PENDING" &&
      currentReservation->status != "APPROVED") {
    throw ReservationServiceError(
        409, "only PENDING or APPROVED reservations can be canceled");
  }

  if (!reservationDAO.cancelMyReservation(studentUserId, reservationId,
                                          cancelReason)) {
    throw ReservationServiceError(409, "reservation cancel failed");
  }

  return reservationDAO.getMyReservationById(studentUserId, reservationId);
}

std::optional<ReservationView> ReservationService::approveReservation(
    std::uint64_t adminUserId,
    std::uint64_t reservationId,
    const std::string& reviewNote) const {
  if (adminUserId == 0 || reservationId == 0) {
    throw ReservationServiceError(400, "adminUserId and reservationId must be >= 1");
  }

  if (reviewNote.size() > 255) {
    throw ReservationServiceError(400, "reviewNote length must be <= 255");
  }

  ReservationDAO reservationDAO(m_databaseConfig);
  const auto reservation = reservationDAO.getReservationById(reservationId);
  if (!reservation.has_value()) {
    return std::nullopt;
  }

  if (reservation->status != "PENDING") {
    throw ReservationServiceError(409, "only PENDING reservations can be approved");
  }

  EquipmentDAO equipmentDAO(m_databaseConfig);
  const auto equipment = equipmentDAO.getById(reservation->equipmentId);
  if (!equipment.has_value()) {
    throw ReservationServiceError(404, "equipment not found");
  }

  if (equipment->status != "AVAILABLE") {
    throw ReservationServiceError(409, "equipment is not available");
  }

  if (reservation->quantity > equipment->availableStock) {
    throw ReservationServiceError(
        409, "requested quantity exceeds current available stock");
  }

  const auto overlapReservedQuantity = reservationDAO.sumReservedQuantityForOverlap(
      reservation->equipmentId, reservation->reservationStartAt,
      reservation->reservationEndAt);
  if (overlapReservedQuantity > equipment->availableStock) {
    throw ReservationServiceError(
        409,
        "reservation cannot be approved because the selected time range is overbooked");
  }

  if (!reservationDAO.reviewReservation(reservationId, adminUserId, "APPROVED",
                                        reviewNote)) {
    throw ReservationServiceError(409, "reservation approve failed");
  }

  return reservationDAO.getReservationById(reservationId);
}

std::optional<ReservationView> ReservationService::rejectReservation(
    std::uint64_t adminUserId,
    std::uint64_t reservationId,
    const std::string& reviewNote) const {
  if (adminUserId == 0 || reservationId == 0) {
    throw ReservationServiceError(400, "adminUserId and reservationId must be >= 1");
  }

  if (reviewNote.size() > 255) {
    throw ReservationServiceError(400, "reviewNote length must be <= 255");
  }

  ReservationDAO reservationDAO(m_databaseConfig);
  const auto reservation = reservationDAO.getReservationById(reservationId);
  if (!reservation.has_value()) {
    return std::nullopt;
  }

  if (reservation->status != "PENDING") {
    throw ReservationServiceError(409, "only PENDING reservations can be rejected");
  }

  if (!reservationDAO.reviewReservation(reservationId, adminUserId, "REJECTED",
                                        reviewNote)) {
    throw ReservationServiceError(409, "reservation reject failed");
  }

  return reservationDAO.getReservationById(reservationId);
}
