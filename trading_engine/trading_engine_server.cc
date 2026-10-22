#include "trading_engine/trading_engine_server.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

#include "absl/status/status.h"
#include "absl/time/clock.h"
#include "trading_engine/order.h"
#include "trading_engine/order_modify.h"
#include "trading_engine/orderbook.h"
#include "trading_engine/types.h"

namespace trading_engine {

TradingEngineServer::TradingEngineServer(Orderbook* orderbook)
    : orderbook_(orderbook) {}

TradingEngineServer::TradingEngineServer(std::unique_ptr<Orderbook> orderbook)
    : orderbook_(orderbook.get()), owned_orderbook_(std::move(orderbook)) {}

TradeResponse TradingEngineServer::AddOrder(const OrderRequest& request) {
  TradeResponse response;
  response.order_id = request.order_id;
  response.timestamp = absl::GetCurrentTimeNanos();

  auto order = std::make_unique<Order>(request.order_type, request.order_id,
                                       request.side, Price(request.price),
                                       Quantity(request.quantity));

  auto trades = orderbook_->AddOrder(std::move(order));

  if (!trades.empty()) {
    response.status = OrderStatus::kFilled;
    response.trades = trades;
    std::uint64_t total_filled = 0;
    std::int64_t total_cost = 0;
    for (const auto& trade : trades) {
      total_filled += trade.GetBidTrade().quantity.value;
      total_cost +=
          trade.GetBidTrade().price.value *
          static_cast<std::int64_t>(trade.GetBidTrade().quantity.value);
    }
    if (total_filled > 0) {
      response.avg_fill_price =
          total_cost / static_cast<std::int64_t>(total_filled);
    }
  } else if (orderbook_->OrderExists(request.order_id)) {
    response.status = OrderStatus::kAccepted;
  } else {
    response.status = OrderStatus::kRejected;
    response.reject_reason = "Order validation failed or immediate cancel";
  }

  return response;
}

CancelOrderResponse TradingEngineServer::CancelOrder(
    const CancelOrderRequest& request) {
  CancelOrderResponse response;
  orderbook_->CancelOrder(request.order_id);
  response.success = true;
  return response;
}

TradeResponse TradingEngineServer::ModifyOrder(
    const ModifyOrderRequest& request) {
  TradeResponse response;
  response.order_id = request.order_id;
  response.timestamp = absl::GetCurrentTimeNanos();

  if (!orderbook_->OrderExists(request.order_id)) {
    response.status = OrderStatus::kRejected;
    response.reject_reason = "Order not found";
    return response;
  }

  OrderModify modify(request.order_id, request.side, Price(request.new_price),
                     Quantity(request.new_quantity));

  auto trades = orderbook_->ModifyOrder(modify);
  response.status =
      trades.empty() ? OrderStatus::kAccepted : OrderStatus::kFilled;
  response.trades = trades;

  return response;
}

OrderbookResponse TradingEngineServer::GetOrderbook(
    const OrderbookRequest& request) {
  OrderbookResponse response;
  response.instrument_id = request.instrument_id;
  response.timestamp = absl::GetCurrentTimeNanos();

  auto infos = orderbook_->GetOrderInfos();
  response.bids = infos.GetBids();
  response.asks = infos.GetAsks();

  if (request.depth > 0) {
    if (response.bids.size() > static_cast<std::size_t>(request.depth)) {
      response.bids.resize(request.depth);
    }
    if (response.asks.size() > static_cast<std::size_t>(request.depth)) {
      response.asks.resize(request.depth);
    }
  }

  return response;
}

absl::Status TradingEngineServer::AddOrder(ServerContext* /*context*/,
                                           const OrderRequest* request,
                                           TradeResponse* response) {
  if (request == nullptr || response == nullptr) {
    return absl::InvalidArgumentError("Null request or response pointer");
  }
  *response = AddOrder(*request);
  return absl::OkStatus();
}

absl::Status TradingEngineServer::CancelOrder(ServerContext* /*context*/,
                                              const CancelOrderRequest* request,
                                              CancelOrderResponse* response) {
  if (request == nullptr || response == nullptr) {
    return absl::InvalidArgumentError("Null request or response pointer");
  }
  *response = CancelOrder(*request);
  return absl::OkStatus();
}

absl::Status TradingEngineServer::ModifyOrder(ServerContext* /*context*/,
                                              const ModifyOrderRequest* request,
                                              TradeResponse* response) {
  if (request == nullptr || response == nullptr) {
    return absl::InvalidArgumentError("Null request or response pointer");
  }
  *response = ModifyOrder(*request);
  return absl::OkStatus();
}

absl::Status TradingEngineServer::GetOrderbook(ServerContext* /*context*/,
                                               const OrderbookRequest* request,
                                               OrderbookResponse* response) {
  if (request == nullptr || response == nullptr) {
    return absl::InvalidArgumentError("Null request or response pointer");
  }
  *response = GetOrderbook(*request);
  return absl::OkStatus();
}

bool TradingEngineServer::RouteOrder(const OrderRequest& request) {
  return incoming_queue_.Push(request);
}

bool TradingEngineServer::ProcessNextRoutedOrder() {
  OrderRequest request;
  if (!incoming_queue_.Pop(request)) {
    return false;
  }
  AddOrder(request);
  return true;
}

}  // namespace trading_engine
