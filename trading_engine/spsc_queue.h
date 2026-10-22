#ifndef TRADING_ENGINE_SPSC_QUEUE_H_
#define TRADING_ENGINE_SPSC_QUEUE_H_

#include <atomic>
#include <cstddef>
#include <utility>
#include <vector>

#include "trading_engine/constants.h"

namespace trading_engine {

template <typename T, std::size_t kCapacity = kDefaultRingBufferSize>
class SpscQueue {
  static_assert((kCapacity & (kCapacity - 1)) == 0,
                "Capacity must be a power of two");

 public:
  SpscQueue() : buffer_(kCapacity) {}

  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;
  SpscQueue(SpscQueue&&) = delete;
  SpscQueue& operator=(SpscQueue&&) = delete;

  ~SpscQueue() = default;

  bool Push(const T& item) {
    const std::size_t current_tail = tail_.load(std::memory_order_relaxed);
    const std::size_t current_head = head_.load(std::memory_order_acquire);

    if ((current_tail - current_head) >= kCapacity) {
      return false;
    }

    buffer_[current_tail & kMask] = item;
    tail_.store(current_tail + 1, std::memory_order_release);
    return true;
  }

  bool Push(T&& item) {
    const std::size_t current_tail = tail_.load(std::memory_order_relaxed);
    const std::size_t current_head = head_.load(std::memory_order_acquire);

    if ((current_tail - current_head) >= kCapacity) {
      return false;
    }

    buffer_[current_tail & kMask] = std::move(item);
    tail_.store(current_tail + 1, std::memory_order_release);
    return true;
  }

  bool Pop(T& out_item) {
    const std::size_t current_head = head_.load(std::memory_order_relaxed);
    const std::size_t current_tail = tail_.load(std::memory_order_acquire);

    if (current_head == current_tail) {
      return false;
    }

    out_item = std::move(buffer_[current_head & kMask]);
    head_.store(current_head + 1, std::memory_order_release);
    return true;
  }

  bool Empty() const {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
  }

  std::size_t Size() const {
    const std::size_t head = head_.load(std::memory_order_acquire);
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    return (tail >= head) ? (tail - head) : 0;
  }

  static constexpr std::size_t Capacity() { return kCapacity; }

 private:
  static constexpr std::size_t kMask = kCapacity - 1;

  alignas(kCacheLineSize) std::atomic<std::size_t> tail_{0};
  alignas(kCacheLineSize) std::atomic<std::size_t> head_{0};
  std::vector<T> buffer_;
};

}  // namespace trading_engine

#endif  // TRADING_ENGINE_SPSC_QUEUE_H_
