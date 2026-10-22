#ifndef TRADING_ENGINE_ORDER_H_
#define TRADING_ENGINE_ORDER_H_

#include <cstdint>

#include "absl/status/status.h"
#include "trading_engine/constants.h"
#include "trading_engine/types.h"

namespace trading_engine {

class Order {
 public:
  Order(OrderType order_type, OrderId order_id, Side side, Price price,
        Quantity quantity);

  Order(OrderType order_type, std::uint64_t order_id, Side side,
        std::int64_t price, std::uint64_t quantity);

  Order(OrderType order_type, std::uint64_t order_id, Side side, Price price,
        Quantity quantity);

  Order(OrderId order_id, Side side, Quantity quantity);

  Order(std::uint64_t order_id, Side side, std::uint64_t quantity);

  OrderId GetOrderId() const { return order_id_; }
  Side GetSide() const { return side_; }
  Price GetPrice() const { return price_; }
  OrderType GetOrderType() const { return order_type_; }
  Quantity GetInitialQuantity() const { return initial_quantity_; }
  Quantity GetRemainingQuantity() const { return remaining_quantity_; }
  Quantity GetFilledQuantity() const {
    return Quantity(initial_quantity_.value - remaining_quantity_.value);
  }
  bool IsFilled() const { return remaining_quantity_.value == 0; }

  bool TryFill(Quantity quantity);
  absl::Status Fill(Quantity quantity);
  absl::Status Fill(std::uint64_t quantity);

  absl::Status ToGoodTillCancel(Price price);
  absl::Status ToGoodTillCancel(std::int64_t price);

 private:
  OrderType order_type_;
  OrderId order_id_;
  Side side_;
  Price price_;
  Quantity initial_quantity_;
  Quantity remaining_quantity_;
};

}  // namespace trading_engine

#endif  // TRADING_ENGINE_ORDER_H_
