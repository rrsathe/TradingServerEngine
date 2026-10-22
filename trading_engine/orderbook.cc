#include "trading_engine/orderbook.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "absl/time/civil_time.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "trading_engine/constants.h"
#include "trading_engine/level_info.h"
#include "trading_engine/order.h"
#include "trading_engine/order_modify.h"
#include "trading_engine/trade.h"
#include "trading_engine/types.h"

namespace trading_engine {

Orderbook::Orderbook()
    : orders_prune_thread_([this] { PruneGoodForDayOrdersLoop(); }) {
  orders_.reserve(kDefaultOrderPoolCapacity);
  data_.reserve(1024);
}

Orderbook::~Orderbook() {
  {
    absl::MutexLock lock(mutex_);
    shutdown_ = true;
    shutdown_cv_.SignalAll();
  }
  if (orders_prune_thread_.joinable()) {
    orders_prune_thread_.join();
  }
}

void Orderbook::PruneGoodForDayOrdersLoop() {
  while (true) {
    const absl::Time now = absl::Now();
    const absl::TimeZone local_tz = absl::LocalTimeZone();
    const absl::CivilSecond cs = absl::ToCivilSecond(now, local_tz);

    absl::CivilSecond target(cs.year(), cs.month(), cs.day(), 16, 0, 0);
    if (cs.hour() >= 16) {
      target = absl::CivilSecond(cs.year(), cs.month(), cs.day() + 1, 16, 0, 0);
    }

    const absl::Time next_close = absl::FromCivil(target, local_tz);
    absl::Duration sleep_duration = next_close - now + absl::Milliseconds(100);
    if (sleep_duration <= absl::ZeroDuration()) {
      sleep_duration = absl::Hours(24);
    }

    {
      absl::MutexLock lock(mutex_);
      if (shutdown_) {
        return;
      }
      shutdown_cv_.WaitWithTimeout(&mutex_, sleep_duration);
      if (shutdown_) {
        return;
      }
    }

    PruneExpiredOrders();
  }
}

void Orderbook::PruneExpiredOrders() {
  absl::MutexLock lock(mutex_);
  std::vector<OrderId> order_ids(good_for_day_orders_.begin(),
                                 good_for_day_orders_.end());
  CancelOrdersInternal(order_ids);
}

void Orderbook::CancelOrdersInternal(const std::vector<OrderId>& order_ids) {
  for (const auto& order_id : order_ids) {
    CancelOrderInternal(order_id);
  }
}

void Orderbook::CancelOrderInternal(OrderId order_id) {
  auto it = orders_.find(order_id);
  if (it == orders_.end()) {
    return;
  }

  Order* order = it->second.order.get();
  auto list_it = it->second.location;
  const Price price = order->GetPrice();
  const Side side = order->GetSide();
  const OrderType order_type = order->GetOrderType();

  if (order_type == OrderType::kGoodForDay) {
    good_for_day_orders_.erase(order_id);
  }

  if (side == Side::kSell) {
    auto ask_it = asks_.find(price);
    if (ask_it != asks_.end()) {
      ask_it->second.erase(list_it);
      if (ask_it->second.empty()) {
        asks_.erase(ask_it);
      }
    }
  } else {
    auto bid_it = bids_.find(price);
    if (bid_it != bids_.end()) {
      bid_it->second.erase(list_it);
      if (bid_it->second.empty()) {
        bids_.erase(bid_it);
      }
    }
  }

  OnOrderCancelled(order);
  orders_.erase(it);
}

void Orderbook::OnOrderCancelled(const Order* order) {
  UpdateLevelData(order->GetPrice(), order->GetRemainingQuantity(),
                  LevelData::Action::kRemove);
}

void Orderbook::OnOrderAdded(const Order* order) {
  UpdateLevelData(order->GetPrice(), order->GetInitialQuantity(),
                  LevelData::Action::kAdd);
}

void Orderbook::OnOrderMatched(Price price, Quantity quantity,
                               bool is_fully_filled) {
  UpdateLevelData(
      price, quantity,
      is_fully_filled ? LevelData::Action::kRemove : LevelData::Action::kMatch);
}

void Orderbook::UpdateLevelData(Price price, Quantity quantity,
                                LevelData::Action action) {
  auto& data = data_[price];

  switch (action) {
    case LevelData::Action::kAdd:
      ++data.count;
      data.quantity += quantity;
      break;
    case LevelData::Action::kRemove:
      if (data.count > 0) {
        --data.count;
      }
      if (data.quantity.value >= quantity.value) {
        data.quantity -= quantity;
      } else {
        data.quantity = Quantity(0);
      }
      break;
    case LevelData::Action::kMatch:
      if (data.quantity.value >= quantity.value) {
        data.quantity -= quantity;
      } else {
        data.quantity = Quantity(0);
      }
      break;
  }

  if (data.count == 0 || data.quantity.value == 0) {
    data_.erase(price);
  }
}

bool Orderbook::CanFullyFill(Side side, Price price, Quantity quantity) const {
  if (!CanMatch(side, price)) {
    return false;
  }

  std::uint64_t remaining_needed = quantity.value;

  if (side == Side::kBuy) {
    for (const auto& [ask_price, orders] : asks_) {
      if (price.IsValid() && ask_price > price) {
        break;
      }
      for (const auto* ask_order : orders) {
        const std::uint64_t available = ask_order->GetRemainingQuantity().value;
        if (remaining_needed <= available) {
          return true;
        }
        remaining_needed -= available;
      }
    }
  } else {
    for (const auto& [bid_price, orders] : bids_) {
      if (price.IsValid() && bid_price < price) {
        break;
      }
      for (const auto* bid_order : orders) {
        const std::uint64_t available = bid_order->GetRemainingQuantity().value;
        if (remaining_needed <= available) {
          return true;
        }
        remaining_needed -= available;
      }
    }
  }

  return false;
}

bool Orderbook::CanMatch(Side side, Price price) const {
  if (side == Side::kBuy) {
    if (asks_.empty()) {
      return false;
    }
    return !price.IsValid() || price >= asks_.begin()->first;
  } else {
    if (bids_.empty()) {
      return false;
    }
    return !price.IsValid() || price <= bids_.begin()->first;
  }
}

Trades Orderbook::MatchOrders() {
  Trades trades;
  trades.reserve(16);

  while (!bids_.empty() && !asks_.empty()) {
    auto& [bid_price, bids] = *bids_.begin();
    auto& [ask_price, asks] = *asks_.begin();

    if (bid_price < ask_price) {
      break;
    }

    while (!bids.empty() && !asks.empty()) {
      Order* bid = bids.front();
      Order* ask = asks.front();

      const Quantity match_qty(std::min(bid->GetRemainingQuantity().value,
                                        ask->GetRemainingQuantity().value));

      bid->TryFill(match_qty);
      ask->TryFill(match_qty);

      trades.emplace_back(
          TradeInfo{bid->GetOrderId(), bid->GetPrice(), match_qty, Side::kBuy},
          TradeInfo{ask->GetOrderId(), ask->GetPrice(), match_qty,
                    Side::kSell});

      OnOrderMatched(bid->GetPrice(), match_qty, bid->IsFilled());
      OnOrderMatched(ask->GetPrice(), match_qty, ask->IsFilled());

      if (bid->IsFilled()) {
        bids.pop_front();
        orders_.erase(bid->GetOrderId());
      }

      if (ask->IsFilled()) {
        asks.pop_front();
        orders_.erase(ask->GetOrderId());
      }
    }

    if (bids.empty()) {
      bids_.erase(bid_price);
      data_.erase(bid_price);
    }

    if (asks.empty()) {
      asks_.erase(ask_price);
      data_.erase(ask_price);
    }
  }

  if (!bids_.empty()) {
    auto& [unused_price, bids] = *bids_.begin();
    if (!bids.empty() &&
        bids.front()->GetOrderType() == OrderType::kFillAndKill) {
      CancelOrderInternal(bids.front()->GetOrderId());
    }
  }

  if (!asks_.empty()) {
    auto& [unused_price, asks] = *asks_.begin();
    if (!asks.empty() &&
        asks.front()->GetOrderType() == OrderType::kFillAndKill) {
      CancelOrderInternal(asks.front()->GetOrderId());
    }
  }

  return trades;
}

Trades Orderbook::AddOrder(std::unique_ptr<Order> order) {
  if (!order || order->GetInitialQuantity().value == 0) {
    return {};
  }

  if (order->GetOrderType() != OrderType::kMarket) {
    if (order->GetPrice().value <= 0 || order->GetPrice().value > kMaxPrice) {
      return {};
    }
  }

  absl::MutexLock lock(mutex_);

  if (orders_.contains(order->GetOrderId())) {
    return {};
  }

  if (order->GetOrderType() == OrderType::kMarket) {
    if (order->GetSide() == Side::kBuy && !asks_.empty()) {
      const auto& [worst_ask, unused_list] = *asks_.rbegin();
      static_cast<void>(order->ToGoodTillCancel(worst_ask));
    } else if (order->GetSide() == Side::kSell && !bids_.empty()) {
      const auto& [worst_bid, unused_list] = *bids_.rbegin();
      static_cast<void>(order->ToGoodTillCancel(worst_bid));
    } else {
      return {};
    }
  }

  if (order->GetOrderType() == OrderType::kGoodForDay) {
    good_for_day_orders_.insert(order->GetOrderId());
  }

  if (order->GetOrderType() == OrderType::kFillAndKill &&
      !CanMatch(order->GetSide(), order->GetPrice())) {
    return {};
  }

  if (order->GetOrderType() == OrderType::kFillOrKill &&
      !CanFullyFill(order->GetSide(), order->GetPrice(),
                    order->GetInitialQuantity())) {
    return {};
  }

  const OrderId id = order->GetOrderId();
  Order* raw_order = order.get();
  auto& orders_at_price =
      (order->GetSide() == Side::kBuy ? bids_[order->GetPrice()]
                                      : asks_[order->GetPrice()]);
  orders_at_price.push_back(raw_order);
  auto list_it = std::prev(orders_at_price.end());

  orders_.emplace(id, OrderEntry{std::move(order), list_it});

  OnOrderAdded(raw_order);

  return MatchOrders();
}

Trades Orderbook::AddOrder(OrderType order_type, std::uint64_t order_id,
                           Side side, std::int64_t price,
                           std::uint64_t quantity) {
  return AddOrder(
      std::make_unique<Order>(order_type, order_id, side, price, quantity));
}

void Orderbook::CancelOrder(OrderId order_id) {
  absl::MutexLock lock(mutex_);
  CancelOrderInternal(order_id);
}

void Orderbook::CancelOrder(std::uint64_t order_id) {
  CancelOrder(OrderId(order_id));
}

Trades Orderbook::ModifyOrder(const OrderModify& order) {
  absl::MutexLock lock(mutex_);

  auto it = orders_.find(order.GetOrderId());
  if (it == orders_.end()) {
    return {};
  }

  const auto order_type = it->second.order->GetOrderType();
  CancelOrderInternal(order.GetOrderId());

  auto new_order = order.ToOrder(order_type);
  const OrderId id = new_order->GetOrderId();
  Order* raw_order = new_order.get();

  if (new_order->GetOrderType() == OrderType::kGoodForDay) {
    good_for_day_orders_.insert(id);
  }

  auto& orders_at_price =
      (new_order->GetSide() == Side::kBuy ? bids_[new_order->GetPrice()]
                                          : asks_[new_order->GetPrice()]);
  orders_at_price.push_back(raw_order);
  auto list_it = std::prev(orders_at_price.end());

  orders_.emplace(id, OrderEntry{std::move(new_order), list_it});

  OnOrderAdded(raw_order);

  return MatchOrders();
}

bool Orderbook::OrderExists(OrderId order_id) const {
  absl::MutexLock lock(mutex_);
  return orders_.contains(order_id);
}

bool Orderbook::OrderExists(std::uint64_t order_id) const {
  return OrderExists(OrderId(order_id));
}

std::size_t Orderbook::Size() const {
  absl::MutexLock lock(mutex_);
  return orders_.size();
}

OrderbookLevelInfos Orderbook::GetOrderInfos() const {
  absl::MutexLock lock(mutex_);

  LevelInfos bid_infos;
  LevelInfos ask_infos;
  bid_infos.reserve(bids_.size());
  ask_infos.reserve(asks_.size());

  for (const auto& [price, orders] : bids_) {
    std::uint64_t total_qty = 0;
    for (const auto* ord : orders) {
      total_qty += ord->GetRemainingQuantity().value;
    }
    bid_infos.emplace_back(price, Quantity(total_qty),
                           static_cast<std::uint32_t>(orders.size()));
  }

  for (const auto& [price, orders] : asks_) {
    std::uint64_t total_qty = 0;
    for (const auto* ord : orders) {
      total_qty += ord->GetRemainingQuantity().value;
    }
    ask_infos.emplace_back(price, Quantity(total_qty),
                           static_cast<std::uint32_t>(orders.size()));
  }

  return OrderbookLevelInfos(std::move(bid_infos), std::move(ask_infos));
}

}  // namespace trading_engine
