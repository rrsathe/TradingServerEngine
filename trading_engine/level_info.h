#ifndef TRADING_ENGINE_LEVEL_INFO_H_
#define TRADING_ENGINE_LEVEL_INFO_H_

#include <cstdint>
#include <utility>
#include <vector>

#include "trading_engine/types.h"

namespace trading_engine {

struct LevelInfo {
  Price price{0};
  Quantity quantity{0};
  std::uint32_t order_count{0};

  LevelInfo() = default;
  LevelInfo(Price p, Quantity q, std::uint32_t count = 1)
      : price(p), quantity(q), order_count(count) {}
};

using LevelInfos = std::vector<LevelInfo>;

class OrderbookLevelInfos {
 public:
  OrderbookLevelInfos(LevelInfos bids, LevelInfos asks)
      : bids_(std::move(bids)), asks_(std::move(asks)) {}

  OrderbookLevelInfos(const OrderbookLevelInfos&) = default;
  OrderbookLevelInfos& operator=(const OrderbookLevelInfos&) = default;
  OrderbookLevelInfos(OrderbookLevelInfos&&) = default;
  OrderbookLevelInfos& operator=(OrderbookLevelInfos&&) = default;
  ~OrderbookLevelInfos() = default;

  const LevelInfos& GetBids() const { return bids_; }
  const LevelInfos& GetAsks() const { return asks_; }

 private:
  LevelInfos bids_;
  LevelInfos asks_;
};

}  // namespace trading_engine

#endif  // TRADING_ENGINE_LEVEL_INFO_H_
