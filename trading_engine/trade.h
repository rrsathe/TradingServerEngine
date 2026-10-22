#ifndef TRADING_ENGINE_TRADE_H_
#define TRADING_ENGINE_TRADE_H_

#include <vector>

#include "trading_engine/types.h"

namespace trading_engine {

struct TradeInfo {
  OrderId order_id;
  Price price;
  Quantity quantity;
  Side side = Side::kBuy;
  Timestamp timestamp{};
};

class Trade {
 public:
  Trade(TradeInfo bid_trade, TradeInfo ask_trade)
      : bid_trade_(bid_trade), ask_trade_(ask_trade) {}

  const TradeInfo& GetBidTrade() const { return bid_trade_; }
  const TradeInfo& GetAskTrade() const { return ask_trade_; }

 private:
  TradeInfo bid_trade_;
  TradeInfo ask_trade_;
};

using Trades = std::vector<Trade>;

}  // namespace trading_engine

#endif  // TRADING_ENGINE_TRADE_H_
