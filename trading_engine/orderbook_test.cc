#include "trading_engine/orderbook.h"

#include <cstdint>
#include <memory>
#include <utility>

#include "gtest/gtest.h"
#include "trading_engine/level_info.h"
#include "trading_engine/order.h"
#include "trading_engine/order_modify.h"
#include "trading_engine/trade.h"
#include "trading_engine/types.h"

namespace trading_engine {
namespace {

class OrderbookTest : public ::testing::Test {
 protected:
  void SetUp() override { orderbook_ = std::make_unique<Orderbook>(); }

  void TearDown() override { orderbook_.reset(); }

  std::unique_ptr<Order> CreateOrder(
      std::uint64_t id, Side side, std::int64_t price, std::uint64_t quantity,
      OrderType type = OrderType::kGoodTillCancel) {
    return std::make_unique<Order>(type, id, side, price, quantity);
  }

  std::unique_ptr<Orderbook> orderbook_;
};

TEST_F(OrderbookTest, EmptyOrderbookInitialization) {
  EXPECT_EQ(orderbook_->Size(), 0);

  auto order_infos = orderbook_->GetOrderInfos();
  EXPECT_TRUE(order_infos.GetBids().empty());
  EXPECT_TRUE(order_infos.GetAsks().empty());
}

TEST_F(OrderbookTest, AddSingleBuyOrder) {
  auto order = CreateOrder(1, Side::kBuy, 100, 1000);
  auto trades = orderbook_->AddOrder(std::move(order));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(orderbook_->Size(), 1);

  auto order_infos = orderbook_->GetOrderInfos();
  EXPECT_EQ(order_infos.GetBids().size(), 1);
  EXPECT_TRUE(order_infos.GetAsks().empty());

  EXPECT_EQ(order_infos.GetBids()[0].price.value, 100);
  EXPECT_EQ(order_infos.GetBids()[0].quantity.value, 1000);
}

TEST_F(OrderbookTest, AddSingleSellOrder) {
  auto order = CreateOrder(1, Side::kSell, 200, 500);
  auto trades = orderbook_->AddOrder(std::move(order));

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(orderbook_->Size(), 1);

  auto order_infos = orderbook_->GetOrderInfos();
  EXPECT_TRUE(order_infos.GetBids().empty());
  EXPECT_EQ(order_infos.GetAsks().size(), 1);

  EXPECT_EQ(order_infos.GetAsks()[0].price.value, 200);
  EXPECT_EQ(order_infos.GetAsks()[0].quantity.value, 500);
}

TEST_F(OrderbookTest, SimpleMatching) {
  auto buy_order = CreateOrder(1, Side::kBuy, 100, 1000);
  auto trades1 = orderbook_->AddOrder(std::move(buy_order));
  EXPECT_TRUE(trades1.empty());

  auto sell_order = CreateOrder(2, Side::kSell, 100, 500);
  auto trades2 = orderbook_->AddOrder(std::move(sell_order));

  EXPECT_EQ(trades2.size(), 1);
  EXPECT_EQ(trades2[0].GetBidTrade().order_id.value, 1);
  EXPECT_EQ(trades2[0].GetAskTrade().order_id.value, 2);
  EXPECT_EQ(trades2[0].GetBidTrade().quantity.value, 500);
  EXPECT_EQ(trades2[0].GetAskTrade().quantity.value, 500);
  EXPECT_EQ(trades2[0].GetBidTrade().price.value, 100);

  EXPECT_EQ(orderbook_->Size(), 1);

  auto order_infos = orderbook_->GetOrderInfos();
  EXPECT_EQ(order_infos.GetBids().size(), 1);
  EXPECT_TRUE(order_infos.GetAsks().empty());
  EXPECT_EQ(order_infos.GetBids()[0].quantity.value, 500);
}

TEST_F(OrderbookTest, CompleteMatching) {
  auto buy_order = CreateOrder(1, Side::kBuy, 100, 1000);
  orderbook_->AddOrder(std::move(buy_order));

  auto sell_order = CreateOrder(2, Side::kSell, 100, 1000);
  auto trades = orderbook_->AddOrder(std::move(sell_order));

  EXPECT_EQ(trades.size(), 1);
  EXPECT_EQ(trades[0].GetBidTrade().quantity.value, 1000);
  EXPECT_EQ(trades[0].GetAskTrade().quantity.value, 1000);
  EXPECT_EQ(orderbook_->Size(), 0);
}

TEST_F(OrderbookTest, TimePriorityFifoMatching) {
  orderbook_->AddOrder(CreateOrder(1, Side::kBuy, 100, 500));
  orderbook_->AddOrder(CreateOrder(2, Side::kBuy, 100, 500));

  auto trades = orderbook_->AddOrder(CreateOrder(3, Side::kSell, 100, 500));
  ASSERT_EQ(trades.size(), 1);
  EXPECT_EQ(trades[0].GetBidTrade().order_id.value, 1);
  EXPECT_EQ(orderbook_->Size(), 1);
  EXPECT_TRUE(orderbook_->OrderExists(2));
}

TEST_F(OrderbookTest, CancelOrderSuccess) {
  orderbook_->AddOrder(CreateOrder(1, Side::kBuy, 100, 1000));
  EXPECT_EQ(orderbook_->Size(), 1);

  orderbook_->CancelOrder(1);
  EXPECT_EQ(orderbook_->Size(), 0);
  EXPECT_FALSE(orderbook_->OrderExists(1));
}

TEST_F(OrderbookTest, CancelNonExistentOrder) {
  orderbook_->CancelOrder(999);
  EXPECT_EQ(orderbook_->Size(), 0);
}

TEST_F(OrderbookTest, ModifyOrderSuccess) {
  orderbook_->AddOrder(CreateOrder(1, Side::kBuy, 100, 1000));

  OrderModify modify(1, Side::kBuy, 110, 800);
  auto trades = orderbook_->ModifyOrder(modify);

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(orderbook_->Size(), 1);

  auto order_infos = orderbook_->GetOrderInfos();
  ASSERT_EQ(order_infos.GetBids().size(), 1);
  EXPECT_EQ(order_infos.GetBids()[0].price.value, 110);
  EXPECT_EQ(order_infos.GetBids()[0].quantity.value, 800);
}

TEST_F(OrderbookTest, ModifyNonExistentOrder) {
  OrderModify modify(999, Side::kBuy, 110, 800);
  auto trades = orderbook_->ModifyOrder(modify);
  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(orderbook_->Size(), 0);
}

TEST_F(OrderbookTest, FillAndKillMatching) {
  orderbook_->AddOrder(CreateOrder(1, Side::kSell, 100, 300));

  auto fak_order =
      CreateOrder(2, Side::kBuy, 100, 1000, OrderType::kFillAndKill);
  auto trades = orderbook_->AddOrder(std::move(fak_order));

  ASSERT_EQ(trades.size(), 1);
  EXPECT_EQ(trades[0].GetBidTrade().quantity.value, 300);
  EXPECT_EQ(orderbook_->Size(), 0);
  EXPECT_FALSE(orderbook_->OrderExists(2));
}

TEST_F(OrderbookTest, FillOrKillSuccessAndFailure) {
  orderbook_->AddOrder(CreateOrder(1, Side::kSell, 100, 300));

  // Cannot fully fill: should cancel without match
  auto fok_fail = CreateOrder(2, Side::kBuy, 100, 500, OrderType::kFillOrKill);
  auto trades_fail = orderbook_->AddOrder(std::move(fok_fail));
  EXPECT_TRUE(trades_fail.empty());
  EXPECT_EQ(orderbook_->Size(), 1);
  EXPECT_FALSE(orderbook_->OrderExists(2));

  // Can fully fill
  auto fok_success =
      CreateOrder(3, Side::kBuy, 100, 300, OrderType::kFillOrKill);
  auto trades_success = orderbook_->AddOrder(std::move(fok_success));
  EXPECT_EQ(trades_success.size(), 1);
  EXPECT_EQ(orderbook_->Size(), 0);
}

TEST_F(OrderbookTest, GoodForDayOrderPruning) {
  orderbook_->AddOrder(
      CreateOrder(1, Side::kBuy, 100, 500, OrderType::kGoodForDay));
  orderbook_->AddOrder(
      CreateOrder(2, Side::kBuy, 100, 500, OrderType::kGoodTillCancel));

  EXPECT_EQ(orderbook_->Size(), 2);

  orderbook_->PruneExpiredOrders();

  EXPECT_EQ(orderbook_->Size(), 1);
  EXPECT_FALSE(orderbook_->OrderExists(1));
  EXPECT_TRUE(orderbook_->OrderExists(2));
}

}  // namespace
}  // namespace trading_engine
