#include "trading_engine/order.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "trading_engine/constants.h"
#include "trading_engine/types.h"

namespace trading_engine {
namespace {

class OrderTest : public ::testing::Test {};

TEST_F(OrderTest, CreateValidOrder) {
  Order order(OrderType::kGoodTillCancel, 123, Side::kBuy, 100, 1000);

  EXPECT_EQ(order.GetOrderId().value, 123);
  EXPECT_EQ(order.GetSide(), Side::kBuy);
  EXPECT_EQ(order.GetPrice().value, 100);
  EXPECT_EQ(order.GetInitialQuantity().value, 1000);
  EXPECT_EQ(order.GetRemainingQuantity().value, 1000);
  EXPECT_EQ(order.GetFilledQuantity().value, 0);
  EXPECT_FALSE(order.IsFilled());
  EXPECT_EQ(order.GetOrderType(), OrderType::kGoodTillCancel);
}

TEST_F(OrderTest, CreateMarketOrder) {
  Order order(456, Side::kSell, 500);

  EXPECT_EQ(order.GetOrderId().value, 456);
  EXPECT_EQ(order.GetSide(), Side::kSell);
  EXPECT_EQ(order.GetPrice().value, kInvalidPrice);
  EXPECT_EQ(order.GetInitialQuantity().value, 500);
  EXPECT_EQ(order.GetRemainingQuantity().value, 500);
  EXPECT_EQ(order.GetOrderType(), OrderType::kMarket);
}

TEST_F(OrderTest, FillOrder) {
  Order order(OrderType::kGoodTillCancel, 789, Side::kBuy, 50, 1000);

  // Fill partially
  auto status1 = order.Fill(300);
  EXPECT_TRUE(status1.ok());
  EXPECT_EQ(order.GetRemainingQuantity().value, 700);
  EXPECT_EQ(order.GetFilledQuantity().value, 300);
  EXPECT_FALSE(order.IsFilled());

  // Fill completely
  auto status2 = order.Fill(700);
  EXPECT_TRUE(status2.ok());
  EXPECT_EQ(order.GetRemainingQuantity().value, 0);
  EXPECT_EQ(order.GetFilledQuantity().value, 1000);
  EXPECT_TRUE(order.IsFilled());
}

TEST_F(OrderTest, FillOrderErrorOnOverfill) {
  Order order(OrderType::kGoodTillCancel, 999, Side::kBuy, 50, 100);

  auto status = order.Fill(150);
  EXPECT_FALSE(status.ok());
  EXPECT_EQ(order.GetRemainingQuantity().value, 100);
}

TEST_F(OrderTest, ConvertToGoodTillCancel) {
  Order order(OrderId(111), Side::kBuy, Quantity(200));

  auto status = order.ToGoodTillCancel(Price(75));
  EXPECT_TRUE(status.ok());

  EXPECT_EQ(order.GetOrderType(), OrderType::kGoodTillCancel);
  EXPECT_EQ(order.GetPrice().value, 75);
}

TEST_F(OrderTest, ConvertNonMarketOrderFails) {
  Order order(OrderType::kGoodTillCancel, 111, Side::kBuy, 50, 200);

  auto status = order.ToGoodTillCancel(Price(75));
  EXPECT_FALSE(status.ok());
}

TEST_F(OrderTest, OrderTypeValidation) {
  Order gtc_order(OrderType::kGoodTillCancel, 1, Side::kBuy, 100, 1000);
  Order gfd_order(OrderType::kGoodForDay, 2, Side::kSell, 200, 500);
  Order fak_order(OrderType::kFillAndKill, 3, Side::kBuy, 150, 250);
  Order fok_order(OrderType::kFillOrKill, 4, Side::kSell, 175, 750);

  EXPECT_EQ(gtc_order.GetOrderType(), OrderType::kGoodTillCancel);
  EXPECT_EQ(gfd_order.GetOrderType(), OrderType::kGoodForDay);
  EXPECT_EQ(fak_order.GetOrderType(), OrderType::kFillAndKill);
  EXPECT_EQ(fok_order.GetOrderType(), OrderType::kFillOrKill);
}

TEST_F(OrderTest, SideValidation) {
  Order buy_order(OrderType::kGoodTillCancel, 1, Side::kBuy, 100, 1000);
  Order sell_order(OrderType::kGoodTillCancel, 2, Side::kSell, 200, 500);

  EXPECT_EQ(buy_order.GetSide(), Side::kBuy);
  EXPECT_EQ(sell_order.GetSide(), Side::kSell);
}

TEST_F(OrderTest, QuantityBehavior) {
  Order order(OrderType::kGoodTillCancel, 1, Side::kBuy, 100, 1000);

  EXPECT_EQ(order.GetInitialQuantity().value, 1000);
  EXPECT_EQ(order.GetRemainingQuantity().value, 1000);
  EXPECT_EQ(order.GetFilledQuantity().value, 0);

  EXPECT_TRUE(order.Fill(250).ok());
  EXPECT_EQ(order.GetInitialQuantity().value, 1000);
  EXPECT_EQ(order.GetRemainingQuantity().value, 750);
  EXPECT_EQ(order.GetFilledQuantity().value, 250);

  EXPECT_TRUE(order.Fill(100).ok());
  EXPECT_EQ(order.GetInitialQuantity().value, 1000);
  EXPECT_EQ(order.GetRemainingQuantity().value, 650);
  EXPECT_EQ(order.GetFilledQuantity().value, 350);
}

}  // namespace
}  // namespace trading_engine
