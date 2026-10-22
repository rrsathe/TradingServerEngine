#!/bin/bash

# Trading Engine Build & Test Script
# Usage: ./build.sh [debug|release|test|benchmark|lint|format|clean|help]

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

BUILD_DIR="build"

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_dependencies() {
    print_status "Checking dependencies..."
    local missing_deps=()

    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    fi

    if ! command -v ninja &> /dev/null && ! command -v make &> /dev/null; then
        missing_deps+=("ninja or make")
    fi

    if ! command -v clang++ &> /dev/null && ! command -v g++ &> /dev/null; then
        missing_deps+=("clang++ or g++")
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        exit 1
    fi

    print_success "Core dependencies found"
}

clean_build() {
    print_status "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    fi
}

configure_build() {
    local build_type="${1:-Release}"
    print_status "Configuring CMake for $build_type build..."
    cmake -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE="$build_type"
}

compile_targets() {
    print_status "Compiling targets..."
    cmake --build "$BUILD_DIR"
    print_success "Build completed successfully"
}

run_tests() {
    print_status "Running test suite via ctest..."
    ctest --test-dir "$BUILD_DIR" --output-on-failure
    print_success "All tests passed"
}

run_benchmarks() {
    print_status "Running Google Benchmark suite..."
    "./$BUILD_DIR/trading_engine_benchmarks"
}

format_code() {
    print_status "Running clang-format -style=Google across codebase..."
    clang-format -style=Google -i trading_engine/*.h trading_engine/*.cc
    print_success "Code formatted according to Google C++ Style Guide"
}

lint_code() {
    print_status "Running clang-tidy with Google profile..."
    clang-tidy -p "$BUILD_DIR" \
        trading_engine/types.cc \
        trading_engine/order.cc \
        trading_engine/orderbook.cc \
        trading_engine/trading_engine_server.cc \
        trading_engine/main.cc
    print_success "Linter completed with zero warnings"
}

show_help() {
    echo "Trading Engine Build Script"
    echo "Usage: ./build.sh [command]"
    echo ""
    echo "Commands:"
    echo "  debug       - Build in Debug mode"
    echo "  release     - Build in Release mode (default)"
    echo "  test        - Build and run unit tests"
    echo "  benchmark   - Build and run Google Benchmark latency suite"
    echo "  format      - Format codebase with clang-format -style=Google"
    echo "  lint        - Run clang-tidy under Google style profile"
    echo "  clean       - Remove build artifacts"
    echo "  help        - Show this help message"
}

check_dependencies

case "${1:-release}" in
    debug)
        configure_build "Debug"
        compile_targets
        ;;
    release)
        configure_build "Release"
        compile_targets
        ;;
    test)
        configure_build "Release"
        compile_targets
        run_tests
        ;;
    benchmark)
        configure_build "Release"
        compile_targets
        run_benchmarks
        ;;
    format)
        format_code
        ;;
    lint)
        configure_build "Release"
        compile_targets
        lint_code
        ;;
    clean)
        clean_build
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        show_help
        exit 1
        ;;
esac
