#ifndef TRADING_ENGINE_ORDERBOOK_H_
#define TRADING_ENGINE_ORDERBOOK_H_

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <thread>
#include <vector>

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "trading_engine/constants.h"
#include "trading_engine/level_info.h"
#include "trading_engine/order.h"
#include "trading_engine/order_modify.h"
#include "trading_engine/trade.h"
#include "trading_engine/types.h"

namespace trading_engine {

class Orderbook {
 public:
  Orderbook();
  Orderbook(const Orderbook&) = delete;
  Orderbook& operator=(const Orderbook&) = delete;
  Orderbook(Orderbook&&) = delete;
  Orderbook& operator=(Orderbook&&) = delete;
  ~Orderbook();

  Trades AddOrder(std::unique_ptr<Order> order);
  Trades AddOrder(OrderType order_type, std::uint64_t order_id, Side side,
                  std::int64_t price, std::uint64_t quantity);

  void CancelOrder(OrderId order_id);
  void CancelOrder(std::uint64_t order_id);

  Trades ModifyOrder(const OrderModify& order);

  bool OrderExists(OrderId order_id) const;
  bool OrderExists(std::uint64_t order_id) const;

  std::size_t Size() const;
  OrderbookLevelInfos GetOrderInfos() const;

  void PruneExpiredOrders();

 private:
  using OrderList = std::list<Order*>;

  struct OrderEntry {
    std::unique_ptr<Order> order;
    OrderList::iterator location;
  };

  struct LevelData {
    Quantity quantity{0};
    std::uint32_t count = 0;

    enum class Action : std::uint8_t {
      kAdd,
      kRemove,
      kMatch,
    };
  };

  void PruneGoodForDayOrdersLoop();
  void CancelOrdersInternal(const std::vector<OrderId>& order_ids);
  void CancelOrderInternal(OrderId order_id);

  void OnOrderCancelled(const Order* order);
  void OnOrderAdded(const Order* order);
  void OnOrderMatched(Price price, Quantity quantity, bool is_fully_filled);
  void UpdateLevelData(Price price, Quantity quantity,
                       LevelData::Action action);

  bool CanFullyFill(Side side, Price price, Quantity quantity) const;
  bool CanMatch(Side side, Price price) const;
  Trades MatchOrders();

  mutable absl::Mutex mutex_;
  absl::CondVar shutdown_cv_;
  bool shutdown_ = false;

  absl::flat_hash_map<Price, LevelData> data_;
  absl::btree_map<Price, OrderList, std::greater<Price>> bids_;
  absl::btree_map<Price, OrderList, std::less<Price>> asks_;
  absl::flat_hash_map<OrderId, OrderEntry> orders_;
  absl::flat_hash_set<OrderId> good_for_day_orders_;

  std::thread orders_prune_thread_;
};

}  // namespace trading_engine

#endif  // TRADING_ENGINE_ORDERBOOK_H_
