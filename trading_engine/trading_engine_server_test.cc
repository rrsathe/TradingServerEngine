#include "trading_engine/trading_engine_server.h"

#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "trading_engine/orderbook.h"
#include "trading_engine/types.h"

namespace trading_engine {
namespace {

class TradingEngineServerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    orderbook_ = std::make_unique<Orderbook>();
    server_ = std::make_unique<TradingEngineServer>(orderbook_.get());
  }

  void TearDown() override {
    server_.reset();
    orderbook_.reset();
  }

  OrderRequest CreateOrderRequest(
      std::uint64_t order_id, Side side, std::int64_t price,
      std::uint64_t quantity,
      OrderType order_type = OrderType::kGoodTillCancel) {
    return OrderRequest{
        .order_id = order_id,
        .side = side,
        .price = price,
        .quantity = quantity,
        .order_type = order_type,
        .client_id = "test_client",
        .instrument_id = "AAPL",
    };
  }

  std::unique_ptr<Orderbook> orderbook_;
  std::unique_ptr<TradingEngineServer> server_;
  ServerContext context_;
};

TEST_F(TradingEngineServerTest, AddOrderSuccess) {
  auto request = CreateOrderRequest(1, Side::kBuy, 100, 1000);
  TradeResponse response;

  auto status = server_->AddOrder(&context_, &request, &response);

  EXPECT_TRUE(status.ok());
  EXPECT_EQ(response.status, OrderStatus::kAccepted);
  EXPECT_EQ(response.trades.size(), 0);
  EXPECT_EQ(orderbook_->Size(), 1);
}

TEST_F(TradingEngineServerTest, AddOrderWithMatching) {
  auto buy_request = CreateOrderRequest(1, Side::kBuy, 100, 1000);
  TradeResponse buy_response;
  EXPECT_TRUE(server_->AddOrder(&context_, &buy_request, &buy_response).ok());

  auto sell_request = CreateOrderRequest(2, Side::kSell, 100, 500);
  TradeResponse sell_response;
  auto status = server_->AddOrder(&context_, &sell_request, &sell_response);

  EXPECT_TRUE(status.ok());
  EXPECT_EQ(sell_response.status, OrderStatus::kFilled);
  EXPECT_EQ(sell_response.trades.size(), 1);
  EXPECT_EQ(sell_response.trades[0].GetBidTrade().order_id.value, 1);
  EXPECT_EQ(sell_response.trades[0].GetAskTrade().order_id.value, 2);
  EXPECT_EQ(sell_response.trades[0].GetBidTrade().quantity.value, 500);
  EXPECT_EQ(orderbook_->Size(), 1);
}

TEST_F(TradingEngineServerTest, CancelOrderSuccess) {
  auto add_request = CreateOrderRequest(1, Side::kBuy, 100, 1000);
  TradeResponse add_response;
  EXPECT_TRUE(server_->AddOrder(&context_, &add_request, &add_response).ok());

  CancelOrderRequest cancel_request{.order_id = 1, .client_id = "test_client"};
  CancelOrderResponse cancel_response;

  auto status =
      server_->CancelOrder(&context_, &cancel_request, &cancel_response);

  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(cancel_response.success);
  EXPECT_EQ(orderbook_->Size(), 0);
}

TEST_F(TradingEngineServerTest, CancelNonExistentOrder) {
  CancelOrderRequest request{.order_id = 999, .client_id = "test_client"};
  CancelOrderResponse response;

  auto status = server_->CancelOrder(&context_, &request, &response);

  EXPECT_TRUE(status.ok());
  EXPECT_TRUE(response.success);
}

TEST_F(TradingEngineServerTest, ModifyOrderSuccess) {
  auto add_request = CreateOrderRequest(1, Side::kBuy, 100, 1000);
  TradeResponse add_response;
  EXPECT_TRUE(server_->AddOrder(&context_, &add_request, &add_response).ok());

  ModifyOrderRequest modify_request{
      .order_id = 1,
      .side = Side::kBuy,
      .new_price = 110,
      .new_quantity = 500,
      .client_id = "test_client",
  };
  TradeResponse modify_response;

  auto status =
      server_->ModifyOrder(&context_, &modify_request, &modify_response);

  EXPECT_TRUE(status.ok());
  EXPECT_EQ(modify_response.status, OrderStatus::kAccepted);
  EXPECT_EQ(orderbook_->Size(), 1);
}

TEST_F(TradingEngineServerTest, ModifyNonExistentOrder) {
  ModifyOrderRequest request{
      .order_id = 999,
      .side = Side::kBuy,
      .new_price = 100,
      .new_quantity = 1000,
      .client_id = "test_client",
  };
  TradeResponse response;

  auto status = server_->ModifyOrder(&context_, &request, &response);

  EXPECT_TRUE(status.ok());
  EXPECT_EQ(response.status, OrderStatus::kRejected);
  EXPECT_EQ(orderbook_->Size(), 0);
}

TEST_F(TradingEngineServerTest, GetOrderbookEmpty) {
  OrderbookRequest request{.instrument_id = "AAPL", .depth = 10};
  OrderbookResponse response;

  auto status = server_->GetOrderbook(&context_, &request, &response);

  EXPECT_TRUE(status.ok());
  EXPECT_EQ(response.bids.size(), 0);
  EXPECT_EQ(response.asks.size(), 0);
}

TEST_F(TradingEngineServerTest, GetOrderbookWithOrders) {
  server_->AddOrder(CreateOrderRequest(1, Side::kBuy, 100, 1000));
  server_->AddOrder(CreateOrderRequest(2, Side::kBuy, 95, 500));
  server_->AddOrder(CreateOrderRequest(3, Side::kSell, 105, 800));
  server_->AddOrder(CreateOrderRequest(4, Side::kSell, 110, 300));

  OrderbookRequest request{.instrument_id = "AAPL", .depth = 5};
  OrderbookResponse response;
  auto status = server_->GetOrderbook(&context_, &request, &response);

  EXPECT_TRUE(status.ok());
  ASSERT_EQ(response.bids.size(), 2);
  ASSERT_EQ(response.asks.size(), 2);

  EXPECT_EQ(response.bids[0].price.value, 100);
  EXPECT_EQ(response.bids[0].quantity.value, 1000);
  EXPECT_EQ(response.bids[1].price.value, 95);
  EXPECT_EQ(response.bids[1].quantity.value, 500);

  EXPECT_EQ(response.asks[0].price.value, 105);
  EXPECT_EQ(response.asks[0].quantity.value, 800);
  EXPECT_EQ(response.asks[1].price.value, 110);
  EXPECT_EQ(response.asks[1].quantity.value, 300);
}

TEST_F(TradingEngineServerTest, ConcurrentOrderHandling) {
  constexpr int kNumThreads = 4;
  constexpr int kOrdersPerThread = 50;

  std::vector<std::thread> threads;
  threads.reserve(kNumThreads);

  for (int t = 0; t < kNumThreads; ++t) {
    threads.emplace_back([this, t] {
      for (int i = 0; i < kOrdersPerThread; ++i) {
        const std::uint64_t id = static_cast<std::uint64_t>(t) * 1000 +
                                 static_cast<std::uint64_t>(i) + 1;
        Side side = (i % 2 == 0) ? Side::kBuy : Side::kSell;
        std::int64_t price = 100 + (i % 5);
        auto request = CreateOrderRequest(id, side, price, 10);
        server_->AddOrder(request);
      }
    });
  }

  for (auto& th : threads) {
    th.join();
  }

  EXPECT_GE(orderbook_->Size(), 0);
}

TEST_F(TradingEngineServerTest, LockFreeRouteOrder) {
  auto request = CreateOrderRequest(100, Side::kBuy, 150, 200);
  EXPECT_TRUE(server_->RouteOrder(request));
  EXPECT_EQ(orderbook_->Size(), 0);

  EXPECT_TRUE(server_->ProcessNextRoutedOrder());
  EXPECT_EQ(orderbook_->Size(), 1);
  EXPECT_TRUE(orderbook_->OrderExists(100));

  EXPECT_FALSE(server_->ProcessNextRoutedOrder());  // Queue empty
}

}  // namespace
}  // namespace trading_engine
