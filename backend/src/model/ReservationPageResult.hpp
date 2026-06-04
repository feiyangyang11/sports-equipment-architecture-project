#ifndef RESERVATION_PAGE_RESULT_HPP
#define RESERVATION_PAGE_RESULT_HPP

#include "model/ReservationView.hpp"

#include <cstdint>
#include <vector>

class ReservationPageResult {
public:
  std::uint32_t pageNo{};
  std::uint32_t pageSize{};
  std::uint64_t total{};
  std::vector<ReservationView> items;
};

#endif
