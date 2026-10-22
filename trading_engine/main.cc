#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>

#include "trading_engine/constants.h"
#include "trading_engine/orderbook.h"
#include "trading_engine/trading_engine_server.h"
#include "trading_engine/types.h"

int main() {
  std::cout << "Starting Trading Engine Server on port "
            << trading_engine::kDefaultPort << "...\n";

  auto orderbook = std::make_unique<trading_engine::Orderbook>();
  trading_engine::TradingEngineServer server(std::move(orderbook));

  // Initialize sample orders
  trading_engine::OrderRequest buy_order{
      .order_id = 1,
      .side = trading_engine::Side::kBuy,
      .price = 100,
      .quantity = 1000,
      .order_type = trading_engine::OrderType::kGoodTillCancel,
      .client_id = "client_1",
      .instrument_id = "AAPL",
  };

  auto buy_response = server.AddOrder(buy_order);
  std::cout << "Order 1 Status: "
            << trading_engine::ToString(buy_response.status) << "\n";

  trading_engine::OrderRequest sell_order{
      .order_id = 2,
      .side = trading_engine::Side::kSell,
      .price = 100,
      .quantity = 500,
      .order_type = trading_engine::OrderType::kGoodTillCancel,
      .client_id = "client_2",
      .instrument_id = "AAPL",
  };

  auto sell_response = server.AddOrder(sell_order);
  std::cout << "Order 2 Status: "
            << trading_engine::ToString(sell_response.status) << "\n";
  std::cout << "Trades generated: " << sell_response.trades.size() << "\n";

  auto book_response = server.GetOrderbook(
      trading_engine::OrderbookRequest{.instrument_id = "AAPL", .depth = 10});
  std::cout << "Active Bid Levels: " << book_response.bids.size()
            << ", Active Ask Levels: " << book_response.asks.size() << "\n";

  std::cout << "Trading Engine Server running successfully.\n";
  return 0;
}
