#ifndef TRADING_ENGINE_CONSTANTS_H_
#define TRADING_ENGINE_CONSTANTS_H_

#include <cstddef>
#include <cstdint>

namespace trading_engine {

inline constexpr std::size_t kCacheLineSize = 64;
inline constexpr std::int64_t kInvalidPrice = -1;
inline constexpr std::int64_t kMaxPrice = 1'000'000'000LL;
inline constexpr std::uint64_t kInvalidOrderId = 0;
inline constexpr std::uint64_t kMaxQuantity = 1'000'000'000ULL;
inline constexpr std::size_t kDefaultRingBufferSize = 65536;
inline constexpr std::size_t kDefaultOrderPoolCapacity = 131072;
inline constexpr std::uint16_t kDefaultPort = 5001;

}  // namespace trading_engine

#endif  // TRADING_ENGINE_CONSTANTS_H_
