#include "trading_engine/order.h"

#include <cstdint>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "trading_engine/constants.h"
#include "trading_engine/types.h"

namespace trading_engine {

Order::Order(OrderType order_type, OrderId order_id, Side side, Price price,
             Quantity quantity)
    : order_type_(order_type),
      order_id_(order_id),
      side_(side),
      price_(price),
      initial_quantity_(quantity),
      remaining_quantity_(quantity) {}

Order::Order(OrderType order_type, std::uint64_t order_id, Side side,
             std::int64_t price, std::uint64_t quantity)
    : Order(order_type, OrderId(order_id), side, Price(price),
            Quantity(quantity)) {}

Order::Order(OrderType order_type, std::uint64_t order_id, Side side,
             Price price, Quantity quantity)
    : Order(order_type, OrderId(order_id), side, price, quantity) {}

Order::Order(OrderId order_id, Side side, Quantity quantity)
    : Order(OrderType::kMarket, order_id, side, Price::Invalid(), quantity) {}

Order::Order(std::uint64_t order_id, Side side, std::uint64_t quantity)
    : Order(OrderId(order_id), side, Quantity(quantity)) {}

bool Order::TryFill(Quantity quantity) {
  if (quantity.value > remaining_quantity_.value) {
    return false;
  }
  remaining_quantity_.value -= quantity.value;
  return true;
}

absl::Status Order::Fill(Quantity quantity) {
  if (quantity.value > remaining_quantity_.value) {
    return absl::InvalidArgumentError(absl::StrCat(
        "Order (", order_id_.value,
        ") cannot be filled for more than its remaining quantity."));
  }
  remaining_quantity_.value -= quantity.value;
  return absl::OkStatus();
}

absl::Status Order::Fill(std::uint64_t quantity) {
  return Fill(Quantity(quantity));
}

absl::Status Order::ToGoodTillCancel(Price price) {
  if (order_type_ != OrderType::kMarket) {
    return absl::FailedPreconditionError(absl::StrCat(
        "Order (", order_id_.value,
        ") cannot have its price adjusted, only market orders can."));
  }
  price_ = price;
  order_type_ = OrderType::kGoodTillCancel;
  return absl::OkStatus();
}

absl::Status Order::ToGoodTillCancel(std::int64_t price) {
  return ToGoodTillCancel(Price(price));
}

}  // namespace trading_engine
