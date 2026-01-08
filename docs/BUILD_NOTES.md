# IMS (Information Management System) Emulation - Build Notes
Version 3.6.2

This document provides detailed build instructions for all supported platforms.

---

## Table of Contents

1. [Requirements](#requirements)
2. [Building on Linux](#building-on-linux)
3. [Building on macOS](#building-on-macos)
4. [Building on Windows (MSVC)](#building-on-windows-msvc)
5. [Building on Windows (MinGW)](#building-on-windows-mingw)
6. [CMake Options](#cmake-options)
7. [IDE Integration](#ide-integration)
8. [Troubleshooting](#troubleshooting)

---

## Requirements

### Compiler Requirements

| Platform | Compiler | Minimum Version |
|----------|----------|-----------------|
| Linux    | GCC      | 11.0            |
| Linux    | Clang    | 14.0            |
| macOS    | Clang    | 14.0            |
| macOS    | GCC      | 11.0            |
| Windows  | MSVC     | 2022 (v17.0)    |
| Windows  | MinGW    | GCC 11.0        |

### Build Tools

- CMake 3.20 or higher
- Make (Linux/macOS) or Ninja
- Visual Studio 2022 (Windows/MSVC)

### No External Dependencies

This project requires only the C++ Standard Library. No external libraries
need to be installed.

---

## Building on Linux

### Ubuntu/Debian

```bash
# Install build tools
sudo apt update
sudo apt install build-essential cmake

# Verify compiler version
g++ --version  # Should be 11.0 or higher

# Clone or extract project
cd IMS-Emulation-Enterprise-v3.6.2

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Run demo
./bin/ims-console-demo
```

### Fedora/RHEL

```bash
# Install build tools
sudo dnf install gcc-c++ cmake make

# Build process is the same as Ubuntu
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Using Clang on Linux

```bash
# Install Clang
sudo apt install clang

# Configure with Clang
cmake -DCMAKE_CXX_COMPILER=clang++ ..
make -j$(nproc)
```

---

## Building on macOS

### Prerequisites

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake via Homebrew
brew install cmake
```

### Building

```bash
cd IMS-Emulation-Enterprise-v3.6.2
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
ctest --output-on-failure
./bin/ims-console-demo
```

### Using GCC on macOS

```bash
# Install GCC via Homebrew
brew install gcc

# Configure with GCC
cmake -DCMAKE_CXX_COMPILER=g++-13 ..
make -j$(sysctl -n hw.ncpu)
```

---

## Building on Windows (MSVC)

### Prerequisites

1. Install Visual Studio 2022 with "Desktop development with C++" workload
2. Install CMake (or use the one bundled with Visual Studio)

### Command Line Build

```batch
cd IMS-Emulation-Enterprise-v3.6.2
mkdir build
cd build

:: Configure
cmake ..

:: Build Release
cmake --build . --config Release

:: Build Debug
cmake --build . --config Debug

:: Run tests
ctest -C Release --output-on-failure

:: Run demo
.\bin\Release\ims-console-demo.exe
```

### Visual Studio IDE

1. Open Visual Studio 2022
2. File -> Open -> Folder -> Select project directory
3. CMake will auto-configure
4. Build -> Build All
5. Select startup item and run

### Developer Command Prompt

```batch
:: Open "Developer Command Prompt for VS 2022"
cd path\to\IMS-Emulation-Enterprise-v3.6.2
mkdir build && cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
```

---

## Building on Windows (MinGW)

### Prerequisites

1. Install MSYS2 from https://www.msys2.org/
2. Open MSYS2 MinGW64 terminal
3. Install toolchain:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make
```

### Building

```bash
cd /path/to/IMS-Emulation-Enterprise-v3.6.2
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j4
ctest --output-on-failure
./bin/ims-console-demo.exe
```

### CLion with MinGW

1. Open CLion
2. File -> Settings -> Build -> Toolchains
3. Add MinGW toolchain pointing to MSYS2 installation
4. File -> Open -> Select project directory
5. Build -> Build Project

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `IMS_BUILD_TESTS` | ON | Build unit tests |
| `IMS_BUILD_EXAMPLES` | ON | Build example applications |
| `IMS_BUILD_BENCHMARKS` | ON | Build benchmark tests |
| `IMS_ENABLE_OPENSSL` | OFF | Enable OpenSSL cryptography |
| `IMS_ENABLE_WARNINGS_AS_ERRORS` | OFF | Treat warnings as errors |
| `CMAKE_BUILD_TYPE` | - | Debug, Release, RelWithDebInfo |

### Example Usage

```bash
# Debug build with tests only
cmake -DCMAKE_BUILD_TYPE=Debug -DIMS_BUILD_EXAMPLES=OFF ..

# Release build without tests
cmake -DCMAKE_BUILD_TYPE=Release -DIMS_BUILD_TESTS=OFF ..

# Strict build (warnings as errors)
cmake -DIMS_ENABLE_WARNINGS_AS_ERRORS=ON ..
```

---

## IDE Integration

### CLion

1. Open CLion
2. File -> Open -> Select project directory
3. CLion auto-detects CMakeLists.txt
4. Build -> Build Project

### Visual Studio Code

1. Install "C/C++" and "CMake Tools" extensions
2. Open project folder
3. CMake Tools will auto-configure
4. Press F7 to build

### Visual Studio 2022

1. File -> Open -> CMake
2. Select CMakeLists.txt
3. Visual Studio configures automatically
4. Build -> Build All

### Qt Creator

1. File -> Open File or Project
2. Select CMakeLists.txt
3. Configure project settings
4. Build -> Build Project

---

## Troubleshooting

### "C++20 not supported"

Ensure your compiler supports C++20:
- GCC 11+
- Clang 14+
- MSVC 2022+

### "std::format not found"

Some older compilers have incomplete C++20 support. Update to:
- GCC 13+
- Clang 16+
- MSVC 2022 17.4+

### MinGW: "error: 'byte' is ambiguous"

Add to CMakeLists.txt or command line:
```cmake
add_compile_definitions(NOMINMAX)
```

### Windows: "LNK2019: unresolved external symbol"

Ensure all libraries are linked correctly. Check that the
build completed without errors.

### macOS: "no member named 'format' in namespace 'std'"

Apple Clang may have limited C++20 support. Use:
```bash
brew install gcc
cmake -DCMAKE_CXX_COMPILER=g++-13 ..
```

### "Permission denied" when running

On Linux/macOS:
```bash
chmod +x ./bin/ims-console-demo
```

### CMake version too old

Install newer CMake:
```bash
# Ubuntu
sudo snap install cmake --classic

# macOS
brew install cmake

# Windows
winget install CMake
```

---

## Build Verification

After building, verify the installation:

```bash
# Run tests
ctest --output-on-failure

# Expected output:
# 100% tests passed, 0 tests failed

# Run demo
./bin/ims-console-demo

# Should display:
# IBM IMS Emulation Enterprise Demo
# ================================
# ...
```

---

## Performance Optimization

### Release Build

For production use, always build in Release mode:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Link-Time Optimization (LTO)

Enable LTO for better performance:
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..
```

### CPU-Specific Optimization

For maximum performance on your specific CPU:
```bash
# GCC/Clang
cmake -DCMAKE_CXX_FLAGS="-march=native" ..
```

---

*Copyright (c) 2025 Bennie Shearer - MIT License*
