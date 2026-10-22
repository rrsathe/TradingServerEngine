#include <cstdint>
#include <memory>
#include <utility>

#include "benchmark/benchmark.h"
#include "trading_engine/order.h"
#include "trading_engine/order_modify.h"
#include "trading_engine/orderbook.h"
#include "trading_engine/types.h"

namespace trading_engine {
namespace {

void BM_AddOrderNoMatch(benchmark::State& state) {
  Orderbook book;
  std::uint64_t id = 1;
  for (auto _ : state) {
    benchmark::DoNotOptimize(book.AddOrder(
        std::make_unique<Order>(OrderType::kGoodTillCancel, id++, Side::kBuy,
                                Price(100), Quantity(10))));
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddOrderNoMatch);

void BM_AddOrderMatch(benchmark::State& state) {
  Orderbook book;
  std::uint64_t id = 1;
  // Pre-seed asks
  for (int i = 0; i < 1000; ++i) {
    book.AddOrder(std::make_unique<Order>(OrderType::kGoodTillCancel, id++,
                                          Side::kSell, Price(100),
                                          Quantity(10)));
  }

  for (auto _ : state) {
    state.PauseTiming();
    auto order = std::make_unique<Order>(OrderType::kGoodTillCancel, id++,
                                         Side::kBuy, Price(100), Quantity(10));
    state.ResumeTiming();
    benchmark::DoNotOptimize(book.AddOrder(std::move(order)));
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddOrderMatch);

void BM_CancelOrder(benchmark::State& state) {
  Orderbook book;
  std::uint64_t id = 1;
  for (auto _ : state) {
    state.PauseTiming();
    book.AddOrder(std::make_unique<Order>(
        OrderType::kGoodTillCancel, id, Side::kBuy, Price(100), Quantity(10)));
    state.ResumeTiming();
    book.CancelOrder(id);
    ++id;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelOrder);

void BM_ModifyOrder(benchmark::State& state) {
  Orderbook book;
  std::uint64_t id = 1;
  book.AddOrder(std::make_unique<Order>(OrderType::kGoodTillCancel, id,
                                        Side::kBuy, Price(100), Quantity(10)));

  std::int64_t new_price = 101;
  for (auto _ : state) {
    OrderModify modify(id, Side::kBuy, Price(new_price++), Quantity(10));
    benchmark::DoNotOptimize(book.ModifyOrder(modify));
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ModifyOrder);

void BM_GetOrderInfos(benchmark::State& state) {
  Orderbook book;
  std::uint64_t id = 1;
  for (int i = 0; i < 50; ++i) {
    book.AddOrder(std::make_unique<Order>(OrderType::kGoodTillCancel, id++,
                                          Side::kBuy, Price(100 - i),
                                          Quantity(100)));
    book.AddOrder(std::make_unique<Order>(OrderType::kGoodTillCancel, id++,
                                          Side::kSell, Price(101 + i),
                                          Quantity(100)));
  }

  for (auto _ : state) {
    benchmark::DoNotOptimize(book.GetOrderInfos());
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetOrderInfos);

}  // namespace
}  // namespace trading_engine

BENCHMARK_MAIN();
