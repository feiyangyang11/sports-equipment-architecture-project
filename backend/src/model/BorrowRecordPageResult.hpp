#ifndef BORROW_RECORD_PAGE_RESULT_HPP
#define BORROW_RECORD_PAGE_RESULT_HPP

#include "model/BorrowRecordView.hpp"

#include <cstdint>
#include <vector>

class BorrowRecordPageResult {
public:
  std::uint32_t pageNo{};
  std::uint32_t pageSize{};
  std::uint64_t total{};
  std::vector<BorrowRecordView> items;
};

#endif
