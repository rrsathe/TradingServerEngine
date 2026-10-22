#ifndef TRADING_ENGINE_TRADING_ENGINE_SERVER_H_
#define TRADING_ENGINE_TRADING_ENGINE_SERVER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "trading_engine/level_info.h"
#include "trading_engine/order.h"
#include "trading_engine/order_modify.h"
#include "trading_engine/orderbook.h"
#include "trading_engine/spsc_queue.h"
#include "trading_engine/trade.h"
#include "trading_engine/types.h"

namespace trading_engine {

struct OrderRequest {
  std::uint64_t order_id = 0;
  Side side = Side::kBuy;
  std::int64_t price = 0;
  std::uint64_t quantity = 0;
  OrderType order_type = OrderType::kGoodTillCancel;
  std::string client_id;
  std::string instrument_id;
};

struct TradeResponse {
  std::uint64_t order_id = 0;
  OrderStatus status = OrderStatus::kAccepted;
  std::string reject_reason;
  Trades trades;
  std::int64_t timestamp = 0;
  std::int64_t avg_fill_price = 0;
  std::uint64_t remaining_quantity = 0;
};

struct CancelOrderRequest {
  std::uint64_t order_id = 0;
  std::string client_id;
};

struct CancelOrderResponse {
  bool success = false;
  std::string reject_reason;
};

struct ModifyOrderRequest {
  std::uint64_t order_id = 0;
  Side side = Side::kBuy;
  std::int64_t new_price = 0;
  std::uint64_t new_quantity = 0;
  std::string client_id;
};

struct OrderbookRequest {
  std::string instrument_id;
  std::int32_t depth = 0;
};

struct OrderbookResponse {
  std::string instrument_id;
  std::int64_t timestamp = 0;
  LevelInfos bids;
  LevelInfos asks;
};

struct ServerContext {};

class TradingEngineServer {
 public:
  explicit TradingEngineServer(Orderbook* orderbook);
  explicit TradingEngineServer(std::unique_ptr<Orderbook> orderbook);

  TradingEngineServer(const TradingEngineServer&) = delete;
  TradingEngineServer& operator=(const TradingEngineServer&) = delete;
  TradingEngineServer(TradingEngineServer&&) = delete;
  TradingEngineServer& operator=(TradingEngineServer&&) = delete;
  ~TradingEngineServer() = default;

  // Direct synchronous API
  TradeResponse AddOrder(const OrderRequest& request);
  CancelOrderResponse CancelOrder(const CancelOrderRequest& request);
  TradeResponse ModifyOrder(const ModifyOrderRequest& request);
  OrderbookResponse GetOrderbook(const OrderbookRequest& request);

  // Context-based RPC API for gRPC compatibility
  absl::Status AddOrder(ServerContext* context, const OrderRequest* request,
                        TradeResponse* response);
  absl::Status CancelOrder(ServerContext* context,
                           const CancelOrderRequest* request,
                           CancelOrderResponse* response);
  absl::Status ModifyOrder(ServerContext* context,
                           const ModifyOrderRequest* request,
                           TradeResponse* response);
  absl::Status GetOrderbook(ServerContext* context,
                            const OrderbookRequest* request,
                            OrderbookResponse* response);

  // Lock-free cross-thread queue routing
  bool RouteOrder(const OrderRequest& request);
  bool ProcessNextRoutedOrder();

  Orderbook* GetOrderbook() const { return orderbook_; }

 private:
  Orderbook* orderbook_;
  std::unique_ptr<Orderbook> owned_orderbook_;
  SpscQueue<OrderRequest> incoming_queue_;
};

}  // namespace trading_engine

#endif  // TRADING_ENGINE_TRADING_ENGINE_SERVER_H_
