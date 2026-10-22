# Trading Engine Server

A high-performance C++ trading engine with gRPC API for order management and matching.

## Features

- **High-Performance Order Matching**: Optimized data structures for microsecond latency
- **Multiple Order Types**: Market, Limit, Fill-or-Kill, Fill-and-Kill, Good-for-Day
- **Thread-Safe Design**: Concurrent order processing with proper synchronization
- **gRPC API**: Modern protocol buffers for client communication
- **Real-time Orderbook**: Live bid/ask level information
- **Automatic Order Expiry**: Good-for-Day orders expire at market close

## Requirements

### System Dependencies

- **C++20** compatible compiler (GCC 10+, Clang 12+)
- **CMake** 3.16 or higher
- **Protocol Buffers** 3.12+
- **gRPC** 1.30+
- **Threads** (pthread on Linux/macOS)

### Ubuntu/Debian Installation
```bash
sudo apt update
sudo apt install cmake build-essential pkg-config
sudo apt install libprotobuf-dev protobuf-compiler
sudo apt install libgrpc++-dev protobuf-compiler-grpc
```

### macOS Installation
```bash
brew install cmake protobuf grpc
```

### Arch Linux Installation
```bash
sudo pacman -S cmake gcc protobuf grpc
```

## Building

### CMake Build (Recommended)
```bash
# Configure build with Ninja (Release)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile all targets
cmake --build build

# Run unit and integration tests
ctest --test-dir build --output-on-failure

# Run latency benchmarks
./build/trading_engine_benchmarks
```

### Bazel Build
```bash
# Build all targets
bazel build //...

# Run test suite
bazel test //:trading_engine_tests

# Run latency benchmarks
bazel run //:trading_engine_benchmarks
```

## Running

### Start the Server
```bash
./build/trading_server
```

The server will start on `localhost:5001` by default.


## API Usage

### gRPC Service Definition

```protobuf
service TradingEngine {
  rpc AddOrder (OrderRequest) returns (TradeResponse);
  rpc CancelOrder (CancelOrderRequest) returns (CancelOrderResponse);  
  rpc ModifyOrder (ModifyOrderRequest) returns (TradeResponse);
  rpc GetOrderbook (OrderbookRequest) returns (OrderbookResponse);
}
```

### Example Client (Python)
```python
import grpc
import trading_optimized_pb2 as trading_pb2
import trading_optimized_pb2_grpc as trading_pb2_grpc

# Connect to server
channel = grpc.insecure_channel('localhost:5001')
stub = trading_pb2_grpc.TradingEngineStub(channel)

# Add buy order
order = trading_pb2.OrderRequest(
    order_id=1,
    side=trading_pb2.BUY,
    price=10000,  # $100.00 (in cents)
    quantity=1000,
    order_type=trading_pb2.GOOD_TILL_CANCEL
)

response = stub.AddOrder(order)
print(f"Order status: {response.status}")
```

## Architecture

### Core Components

- **Orderbook**: Central matching engine with price-time priority
- **TradingEngineServer**: gRPC service implementation  
- **Order Management**: Order lifecycle and validation
- **Threading**: Concurrent order processing and background tasks

### Data Structures

- **Price-Time Priority**: `std::map` with custom comparators
- **Order Storage**: Hash maps for O(1) order lookup
- **Memory Efficient**: Optimized protobuf messages (16 bytes per trade)

### Performance Characteristics

- **Latency**: ~10-50 microseconds per order operation
- **Throughput**: 100,000+ orders per second
- **Memory**: <1GB for 1M active orders
- **Concurrent**: Thread-safe for multiple clients

## Development

### Project Structure
```
TradingEngineServer/
├── BUILD.bazel              # Bazel build targets
├── CMakeLists.txt           # Modern target-based CMake configuration
├── MODULE.bazel             # Bzlmod module dependencies
├── README.md                # Project documentation
├── proto/                   # Canonical protobuf definitions
│   └── trading.proto
└── trading_engine/          # Google C++ Style codebase
    ├── constants.h
    ├── level_info.h
    ├── main.cc
    ├── order.cc
    ├── order.h
    ├── order_modify.h
    ├── order_test.cc
    ├── orderbook.cc
    ├── orderbook.h
    ├── orderbook_benchmark.cc
    ├── orderbook_test.cc
    ├── spsc_queue.h
    ├── trade.h
    ├── trading_engine_server.cc
    ├── trading_engine_server.h
    ├── trading_engine_server_test.cc
    ├── types.cc
    ├── types.h
    └── types_test.cc
```

### Adding Features

1. **New Order Types**: Extend `OrderType` enum and matching logic
2. **Risk Management**: Add position limits in `AddOrder`
3. **Market Data**: Implement orderbook streaming
4. **Persistence**: Add database integration for order recovery

### Testing
```bash
# Run GoogleTest suite via CTest
ctest --test-dir build --output-on-failure

# Run test suite via Bazel
bazel test //:trading_engine_tests
```

## Performance Tuning

### Compiler Optimizations
```bash
# Maximum performance build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Runtime Tuning
- Increase file descriptor limits: `ulimit -n 65536`
- Set CPU affinity: `taskset -c 0,1 ./trading_server`
- Use huge pages for memory allocation
- Disable CPU frequency scaling

### Profiling
```bash
# Build with profiling
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-pg"
make

# Run and profile
./build/trading_server &
gprof ./build/trading_server gmon.out > profile.txt
```

## Troubleshooting

### Common Build Issues

**gRPC not found:**
```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

**Protobuf version mismatch:**
```bash
pkg-config --modversion protobuf
protoc --version
```

**Missing C++20 support:**
```bash
g++ --version  # Should be 10+ 
clang++ --version  # Should be 12+
```

### Runtime Issues

**Port already in use:**
```bash
lsof -i :5001
# Kill existing process
```

**Permission denied:**
```bash
# Check if port < 1024 requires sudo
sudo ./trading_server
```

## License

MIT License - see LICENSE file for details.

## Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Support

For questions and support:
- Create an issue on GitHub
- Check the troubleshooting section
- Review the API documentation
