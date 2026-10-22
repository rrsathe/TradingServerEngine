#include "trading_engine/types.h"

#include <cstdint>

#include "absl/container/flat_hash_map.h"
#include "gtest/gtest.h"
#include "trading_engine/spsc_queue.h"

namespace trading_engine {
namespace {

TEST(TypesTest, StrongTypeComparisonsAndArithmetic) {
  OrderId id1(100);
  OrderId id2(100);
  OrderId id3(200);

  EXPECT_EQ(id1, id2);
  EXPECT_LT(id1, id3);
  EXPECT_TRUE(id1.IsValid());

  OrderId invalid_id;
  EXPECT_FALSE(invalid_id.IsValid());

  Price p1(1500);
  Price p2(500);
  EXPECT_EQ((p1 + p2).value, 2000);
  EXPECT_EQ((p1 - p2).value, 1000);
  EXPECT_TRUE(p1 > p2);

  Quantity q1(10);
  Quantity q2(5);
  EXPECT_EQ((q1 + q2).value, 15);
  EXPECT_EQ((q1 - q2).value, 5);
}

TEST(TypesTest, AbseilHashSupport) {
  absl::flat_hash_map<OrderId, int> id_map;
  id_map[OrderId(10)] = 100;
  EXPECT_EQ(id_map[OrderId(10)], 100);

  absl::flat_hash_map<Price, int> price_map;
  price_map[Price(50)] = 200;
  EXPECT_EQ(price_map[Price(50)], 200);

  absl::flat_hash_map<Quantity, int> qty_map;
  qty_map[Quantity(500)] = 300;
  EXPECT_EQ(qty_map[Quantity(500)], 300);
}

TEST(TypesTest, ToStringConversions) {
  EXPECT_EQ(ToString(Side::kBuy), "BUY");
  EXPECT_EQ(ToString(Side::kSell), "SELL");

  EXPECT_EQ(ToString(OrderType::kGoodTillCancel), "GOOD_TILL_CANCEL");
  EXPECT_EQ(ToString(OrderType::kFillAndKill), "FILL_AND_KILL");
  EXPECT_EQ(ToString(OrderType::kFillOrKill), "FILL_OR_KILL");
  EXPECT_EQ(ToString(OrderType::kGoodForDay), "GOOD_FOR_DAY");
  EXPECT_EQ(ToString(OrderType::kMarket), "MARKET");

  EXPECT_EQ(ToString(OrderStatus::kAccepted), "ACCEPTED");
  EXPECT_EQ(ToString(OrderStatus::kRejected), "REJECTED");
  EXPECT_EQ(ToString(OrderStatus::kFilled), "FILLED");
  EXPECT_EQ(ToString(OrderStatus::kPartiallyFilled), "PARTIALLY_FILLED");
  EXPECT_EQ(ToString(OrderStatus::kCancelled), "CANCELLED");

  EXPECT_EQ(ToString(RejectReason::kNone), "NONE");
  EXPECT_EQ(ToString(RejectReason::kOrderNotFound), "ORDER_NOT_FOUND");
}

TEST(TypesTest, SpscQueueBasicOperations) {
  SpscQueue<int, 8> queue;
  EXPECT_TRUE(queue.Empty());
  EXPECT_EQ(queue.Size(), 0);

  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(queue.Push(i * 10));
  }
  EXPECT_EQ(queue.Size(), 8);
  EXPECT_FALSE(queue.Push(999));  // Full

  int val = 0;
  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(queue.Pop(val));
    EXPECT_EQ(val, i * 10);
  }
  EXPECT_TRUE(queue.Empty());
  EXPECT_FALSE(queue.Pop(val));
}

TEST(TypesTest, SymbolView) {
  Symbol s("AAPL");
  EXPECT_EQ(s.View(), "AAPL");
}

}  // namespace
}  // namespace trading_engine
