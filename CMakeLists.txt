cmake_minimum_required(VERSION 3.20)

project(TradingEngineServer 
    VERSION 2.0.0
    DESCRIPTION "High-Performance Deterministic Trading Engine Server"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enforce no exceptions as required by the Google C++ Style Guide
add_compile_options(-fno-exceptions -Wall -Wextra -Wpedantic)

# Dependencies
find_package(Threads REQUIRED)
find_package(absl CONFIG REQUIRED)

# Core Trading Engine Library
add_library(trading_engine STATIC
    trading_engine/types.cc
    trading_engine/order.cc
    trading_engine/orderbook.cc
    trading_engine/trading_engine_server.cc
)

target_include_directories(trading_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(trading_engine
    PUBLIC
        absl::status
        absl::statusor
        absl::flat_hash_map
        absl::flat_hash_set
        absl::btree
        absl::strings
        absl::time
        absl::synchronization
        Threads::Threads
)

target_compile_features(trading_engine PUBLIC cxx_std_20)

# Server Executable
add_executable(trading_server trading_engine/main.cc)
target_link_libraries(trading_server PRIVATE trading_engine)

# Unit & Integration Tests (GoogleTest & GoogleMock)
enable_testing()
find_package(GTest REQUIRED)

add_executable(trading_engine_tests
    trading_engine/types_test.cc
    trading_engine/order_test.cc
    trading_engine/orderbook_test.cc
    trading_engine/trading_engine_server_test.cc
)

target_link_libraries(trading_engine_tests
    PRIVATE
        trading_engine
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
)

include(GoogleTest)
gtest_discover_tests(trading_engine_tests)

# Performance Benchmarks (Google Benchmark)
include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

find_package(benchmark CONFIG QUIET)
if(NOT benchmark_FOUND)
    message(STATUS "Google Benchmark not found on system, fetching via FetchContent...")
    FetchContent_Declare(
        google_benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.1
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(google_benchmark)
endif()

add_executable(trading_engine_benchmarks
    trading_engine/orderbook_benchmark.cc
)

target_link_libraries(trading_engine_benchmarks
    PRIVATE
        trading_engine
        benchmark::benchmark
        Threads::Threads
)
