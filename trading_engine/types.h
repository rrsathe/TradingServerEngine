#ifndef TRADING_ENGINE_TYPES_H_
#define TRADING_ENGINE_TYPES_H_

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "absl/hash/hash.h"
#include "absl/strings/string_view.h"
#include "trading_engine/constants.h"

namespace trading_engine {

enum class Side : std::uint8_t {
  kBuy = 1,
  kSell = 2,
};

absl::string_view ToString(Side side);

enum class OrderType : std::uint8_t {
  kGoodTillCancel = 1,
  kFillAndKill = 2,
  kFillOrKill = 3,
  kGoodForDay = 4,
  kMarket = 5,
};

absl::string_view ToString(OrderType type);

enum class OrderStatus : std::uint8_t {
  kAccepted = 1,
  kRejected = 2,
  kFilled = 3,
  kPartiallyFilled = 4,
  kCancelled = 5,
};

absl::string_view ToString(OrderStatus status);

enum class RejectReason : std::uint8_t {
  kNone = 0,
  kOrderNotFound = 1,
  kDuplicateOrderId = 2,
  kInvalidPrice = 3,
  kInvalidQuantity = 4,
  kInsufficientLiquidity = 5,
  kMalformedPayload = 6,
  kQueueFull = 7,
  kUnknownOrderType = 8,
  kUnknownSide = 9,
};

absl::string_view ToString(RejectReason reason);

struct OrderId {
  std::uint64_t value = kInvalidOrderId;

  constexpr OrderId() = default;
  constexpr explicit OrderId(std::uint64_t v) : value(v) {}

  constexpr bool IsValid() const { return value != kInvalidOrderId; }

  constexpr auto operator<=>(const OrderId&) const = default;

  template <typename H>
  friend H AbslHashValue(H h, const OrderId& id) {
    return H::combine(std::move(h), id.value);
  }
};

struct Price {
  std::int64_t value = kInvalidPrice;

  constexpr Price() = default;
  constexpr explicit Price(std::int64_t v) : value(v) {}

  static constexpr Price Invalid() { return Price(kInvalidPrice); }
  constexpr bool IsValid() const { return value > 0 && value <= kMaxPrice; }

  constexpr auto operator<=>(const Price&) const = default;

  constexpr Price operator+(Price other) const {
    return Price(value + other.value);
  }
  constexpr Price operator-(Price other) const {
    return Price(value - other.value);
  }
  constexpr Price& operator+=(Price other) {
    value += other.value;
    return *this;
  }
  constexpr Price& operator-=(Price other) {
    value -= other.value;
    return *this;
  }

  template <typename H>
  friend H AbslHashValue(H h, const Price& p) {
    return H::combine(std::move(h), p.value);
  }
};

struct Quantity {
  std::uint64_t value = 0;

  constexpr Quantity() = default;
  constexpr explicit Quantity(std::uint64_t v) : value(v) {}

  constexpr bool IsValid() const { return value > 0 && value <= kMaxQuantity; }

  constexpr auto operator<=>(const Quantity&) const = default;

  constexpr Quantity operator+(Quantity other) const {
    return Quantity(value + other.value);
  }
  constexpr Quantity operator-(Quantity other) const {
    return Quantity(value - other.value);
  }
  constexpr Quantity& operator+=(Quantity other) {
    value += other.value;
    return *this;
  }
  constexpr Quantity& operator-=(Quantity other) {
    value -= other.value;
    return *this;
  }

  template <typename H>
  friend H AbslHashValue(H h, const Quantity& q) {
    return H::combine(std::move(h), q.value);
  }
};

struct Timestamp {
  std::uint64_t nanoseconds = 0;

  constexpr Timestamp() = default;
  constexpr explicit Timestamp(std::uint64_t ns) : nanoseconds(ns) {}

  constexpr auto operator<=>(const Timestamp&) const = default;
};

struct Symbol {
  std::array<char, 8> data{};

  constexpr Symbol() { data.fill('\0'); }
  constexpr explicit Symbol(absl::string_view sv) {
    data.fill('\0');
    const std::size_t copy_len = std::min(sv.size(), data.size() - 1);
    for (std::size_t i = 0; i < copy_len; ++i) {
      data[i] = sv[i];
    }
  }

  absl::string_view View() const {
    std::size_t len = 0;
    while (len < data.size() && data[len] != '\0') {
      ++len;
    }
    return absl::string_view(data.data(), len);
  }

  constexpr auto operator<=>(const Symbol&) const = default;
};

}  // namespace trading_engine

#endif  // TRADING_ENGINE_TYPES_H_
