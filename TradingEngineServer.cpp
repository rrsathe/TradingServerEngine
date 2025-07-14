#include "TradingEngineServer.hpp"

grpc::Status TradingEngineServer::AddOrder(grpc::ServerContext * /*context*/, const trading::OrderRequest *request,
										   trading::TradeResponse *response) {

	auto order = std::make_shared<Order>(
		ParseOrderType(request->order_type()),
		request->order_id(),
		ParseSide(request->side()),
		request->price(),
		request->quantity());

	const auto trades = orderbook_->AddOrder(order);

	// Set status based on whether order was filled or just placed
	if (trades.empty()) [[likely]] {
		// Order was added to orderbook without matches
		response->set_status(::trading::OrderStatus::ACCEPTED);
	} else {
		// Order was matched and generated trades
		response->set_status(::trading::OrderStatus::FILLED);

		// Use structured binding and range-based loop for modern C++20
		for (const auto &trade : trades) {
			const auto &[bidOrderId, bidPrice, bidQuantity] = trade.GetBidTrade();
			const auto &[askOrderId, askPrice, askQuantity] = trade.GetAskTrade();

			auto *bidTradeInfo = response->add_trades();
			bidTradeInfo->set_order_id(bidOrderId);
			bidTradeInfo->set_price(bidPrice);
			bidTradeInfo->set_quantity(bidQuantity);

			auto *askTradeInfo = response->add_trades();
			askTradeInfo->set_order_id(askOrderId);
			askTradeInfo->set_price(askPrice);
			askTradeInfo->set_quantity(askQuantity);
		}
	}

	return grpc::Status::OK;
}

grpc::Status TradingEngineServer::CancelOrder(grpc::ServerContext * /*context*/, const trading::CancelOrderRequest *request,
											  trading::CancelOrderResponse *response) {
	orderbook_->CancelOrder(request->order_id());
	response->set_success(true);

	return grpc::Status::OK;
}

grpc::Status TradingEngineServer::ModifyOrder(grpc::ServerContext * /*context*/, const trading::ModifyOrderRequest *request,
											  trading::TradeResponse *response) {
	// Check if order exists first
	if (!orderbook_->OrderExists(request->order_id())) [[unlikely]] {
		response->set_status(::trading::OrderStatus::REJECTED);
		return grpc::Status::OK;
	}

	const OrderModify order{
		request->order_id(),
		ParseSide(request->side()),
		request->new_price(),
		request->new_quantity()};

	const auto trades = orderbook_->ModifyOrder(order);

	// Set status based on whether order modification resulted in trades
	if (trades.empty()) [[likely]] {
		// Order was modified successfully without matches
		response->set_status(::trading::OrderStatus::ACCEPTED);
	} else {
		// Order modification resulted in trades
		response->set_status(::trading::OrderStatus::FILLED);

		// Use structured binding and range-based loop for modern C++20
		for (const auto &trade : trades) {
			const auto &[bidOrderId, bidPrice, bidQuantity] = trade.GetBidTrade();
			const auto &[askOrderId, askPrice, askQuantity] = trade.GetAskTrade();

			auto *bidTradeInfo = response->add_trades();
			bidTradeInfo->set_order_id(bidOrderId);
			bidTradeInfo->set_price(bidPrice);
			bidTradeInfo->set_quantity(bidQuantity);

			auto *askTradeInfo = response->add_trades();
			askTradeInfo->set_order_id(askOrderId);
			askTradeInfo->set_price(askPrice);
			askTradeInfo->set_quantity(askQuantity);
		}
	}

	return grpc::Status::OK;
}

grpc::Status TradingEngineServer::GetOrderbook(grpc::ServerContext * /*context*/, const trading::OrderbookRequest * /*request*/,
											   trading::OrderbookResponse *response) {
	const auto orderInfos = orderbook_->GetOrderInfos();

	const auto &bids = orderInfos.GetBids();
	const auto &asks = orderInfos.GetAsks();

	// Use structured binding for modern C++20 style
	for (const auto &[price, quantity] : bids) {
		auto *bidInfo = response->add_bids();
		bidInfo->set_price(price);
		bidInfo->set_quantity(quantity);
	}

	for (const auto &[price, quantity] : asks) {
		auto *askInfo = response->add_asks();
		askInfo->set_price(price);
		askInfo->set_quantity(quantity);
	}

	return grpc::Status::OK;
}

[[nodiscard]] OrderType TradingEngineServer::ParseOrderType(trading::OrderType type) {
	switch (type) {
		using enum trading::OrderType;
	case MARKET:
		return OrderType::Market;
	case GOOD_FOR_DAY:
		return OrderType::GoodForDay;
	case FILL_OR_KILL:
		return OrderType::FillOrKill;
	case FILL_AND_KILL:
		return OrderType::FillAndKill;
	case GOOD_TILL_CANCEL:
		return OrderType::GoodTillCancel;
	default:
		throw std::invalid_argument("Unknown order type");
	}
}

[[nodiscard]] Side TradingEngineServer::ParseSide(::trading::Side side) {
	switch (side) {
		using enum ::trading::Side;
	case BUY:
		return Side::Buy;
	case SELL:
		return Side::Sell;
	default:
		throw std::invalid_argument("Unknown side");
	}
}