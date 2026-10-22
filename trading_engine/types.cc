#include "trading_engine/types.h"

#include "absl/strings/string_view.h"

namespace trading_engine {

absl::string_view ToString(Side side) {
  switch (side) {
    case Side::kBuy:
      return "BUY";
    case Side::kSell:
      return "SELL";
  }
  return "UNKNOWN";
}

absl::string_view ToString(OrderType type) {
  switch (type) {
    case OrderType::kGoodTillCancel:
      return "GOOD_TILL_CANCEL";
    case OrderType::kFillAndKill:
      return "FILL_AND_KILL";
    case OrderType::kFillOrKill:
      return "FILL_OR_KILL";
    case OrderType::kGoodForDay:
      return "GOOD_FOR_DAY";
    case OrderType::kMarket:
      return "MARKET";
  }
  return "UNKNOWN";
}

absl::string_view ToString(OrderStatus status) {
  switch (status) {
    case OrderStatus::kAccepted:
      return "ACCEPTED";
    case OrderStatus::kRejected:
      return "REJECTED";
    case OrderStatus::kFilled:
      return "FILLED";
    case OrderStatus::kPartiallyFilled:
      return "PARTIALLY_FILLED";
    case OrderStatus::kCancelled:
      return "CANCELLED";
  }
  return "UNKNOWN";
}

absl::string_view ToString(RejectReason reason) {
  switch (reason) {
    case RejectReason::kNone:
      return "NONE";
    case RejectReason::kOrderNotFound:
      return "ORDER_NOT_FOUND";
    case RejectReason::kDuplicateOrderId:
      return "DUPLICATE_ORDER_ID";
    case RejectReason::kInvalidPrice:
      return "INVALID_PRICE";
    case RejectReason::kInvalidQuantity:
      return "INVALID_QUANTITY";
    case RejectReason::kInsufficientLiquidity:
      return "INSUFFICIENT_LIQUIDITY";
    case RejectReason::kMalformedPayload:
      return "MALFORMED_PAYLOAD";
    case RejectReason::kQueueFull:
      return "QUEUE_FULL";
    case RejectReason::kUnknownOrderType:
      return "UNKNOWN_ORDER_TYPE";
    case RejectReason::kUnknownSide:
      return "UNKNOWN_SIDE";
  }
  return "UNKNOWN";
}

}  // namespace trading_engine
