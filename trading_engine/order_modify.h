#ifndef TRADING_ENGINE_ORDER_MODIFY_H_
#define TRADING_ENGINE_ORDER_MODIFY_H_

#include <cstdint>
#include <memory>

#include "trading_engine/order.h"
#include "trading_engine/types.h"

namespace trading_engine {

class OrderModify {
 public:
  OrderModify(OrderId order_id, Side side, Price price, Quantity quantity)
      : order_id_(order_id), side_(side), price_(price), quantity_(quantity) {}

  OrderModify(std::uint64_t order_id, Side side, std::int64_t price,
              std::uint64_t quantity)
      : order_id_(OrderId(order_id)),
        side_(side),
        price_(Price(price)),
        quantity_(Quantity(quantity)) {}

  OrderModify(std::uint64_t order_id, Side side, Price price, Quantity quantity)
      : order_id_(OrderId(order_id)),
        side_(side),
        price_(price),
        quantity_(quantity) {}

  OrderId GetOrderId() const { return order_id_; }
  Side GetSide() const { return side_; }
  Price GetPrice() const { return price_; }
  Quantity GetQuantity() const { return quantity_; }

  std::unique_ptr<Order> ToOrder(OrderType order_type) const {
    return std::make_unique<Order>(order_type, order_id_, side_, price_,
                                   quantity_);
  }

 private:
  OrderId order_id_;
  Side side_;
  Price price_;
  Quantity quantity_;
};

}  // namespace trading_engine

#endif  // TRADING_ENGINE_ORDER_MODIFY_H_
