# =============================================================================
# IBM IMS (Information Management System) Emulation Enterprise - Main CMakeLists.txt
# Version: 3.6.3
# =============================================================================
#
# Enterprise-grade IBM IMS/VSAM/CICS mainframe emulation system.
# Cross-platform support: Windows, Linux, macOS
# C++20 required.
#
# Copyright (c) 2025 Bennie Shearer
# MIT License - See LICENSE file for details
# =============================================================================

cmake_minimum_required(VERSION 3.20)

# Project definition
project(IBM_IMS_Emulation
    VERSION 3.6.3
    DESCRIPTION "IBM IMS (Information Management System) Emulation"
    LANGUAGES CXX
)

# =============================================================================
# Build Options
# =============================================================================

option(IMS_BUILD_TESTS "Build unit tests" ON)
option(IMS_BUILD_EXAMPLES "Build example applications" ON)
option(IMS_BUILD_BENCHMARKS "Build benchmark tests" ON)
option(IMS_ENABLE_OPENSSL "Enable OpenSSL for cryptography" OFF)
option(IMS_ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# =============================================================================
# C++ Standard
# =============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Platform Detection
# =============================================================================

if(WIN32)
    set(IMS_PLATFORM "Windows")
    set(IMS_PLATFORM_WINDOWS TRUE)
    add_compile_definitions(IMS_PLATFORM_WINDOWS=1)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(_WIN32_WINNT=0x0601)
elseif(APPLE)
    set(IMS_PLATFORM "macOS")
    set(IMS_PLATFORM_MACOS TRUE)
    add_compile_definitions(IMS_PLATFORM_MACOS=1)
else()
    set(IMS_PLATFORM "Linux")
    set(IMS_PLATFORM_LINUX TRUE)
    add_compile_definitions(IMS_PLATFORM_LINUX=1)
endif()

message(STATUS "")
message(STATUS "=================================================")
message(STATUS "  IBM IMS (Information Management System) Emulation Enterprise v${PROJECT_VERSION}")
message(STATUS "=================================================")
message(STATUS "Building for platform: ${IMS_PLATFORM}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")

# =============================================================================
# Compiler Detection and Settings
# =============================================================================

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
    
    if(IMS_ENABLE_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
    
    add_compile_options(/MP)
    message(STATUS "Compiler: MSVC ${MSVC_VERSION}")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    
    if(IMS_ENABLE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O2)
    endif()
    
    message(STATUS "Compiler: GCC ${CMAKE_CXX_COMPILER_VERSION}")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    
    if(IMS_ENABLE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O2)
    endif()
    
    message(STATUS "Compiler: Clang ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# =============================================================================
# Threading
# =============================================================================

find_package(Threads REQUIRED)

# =============================================================================
# SIMD Detection
# =============================================================================

include(CheckCXXCompilerFlag)

if(NOT MSVC)
    check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)
    check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
    
    if(COMPILER_SUPPORTS_SSE42)
        set(IMS_HAS_SSE42 TRUE)
    endif()
    
    if(COMPILER_SUPPORTS_AVX2)
        set(IMS_HAS_AVX2 TRUE)
    endif()
endif()

# =============================================================================
# Output Directories
# =============================================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# =============================================================================
# Configuration Header
# =============================================================================

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ims_config.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/ims_config.hpp"
    @ONLY
)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/include")

# =============================================================================
# Print Configuration Summary
# =============================================================================

message(STATUS "")
message(STATUS "=== Build Configuration Summary ===")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Platform:       ${IMS_PLATFORM}")
message(STATUS "  C++ Standard:   C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests:    ${IMS_BUILD_TESTS}")
message(STATUS "  Build Examples: ${IMS_BUILD_EXAMPLES}")
message(STATUS "  Build Benchmarks: ${IMS_BUILD_BENCHMARKS}")
message(STATUS "  OpenSSL:        ${IMS_ENABLE_OPENSSL}")
message(STATUS "")

# =============================================================================
# Add Libraries
# =============================================================================

add_subdirectory(libs/common)
add_subdirectory(libs/security)
add_subdirectory(libs/master-catalog)
add_subdirectory(libs/vsam)
add_subdirectory(libs/dfsmshsm)
add_subdirectory(libs/ims-core)
add_subdirectory(libs/gdg)

# =============================================================================
# Add Console Demo Application
# =============================================================================

add_executable(ims-console-demo apps/console-demo/main.cpp)
target_include_directories(ims-console-demo PRIVATE
    ${CMAKE_SOURCE_DIR}/libs/common/include
    ${CMAKE_SOURCE_DIR}/libs/security/include
    ${CMAKE_SOURCE_DIR}/libs/master-catalog/include
    ${CMAKE_SOURCE_DIR}/libs/vsam/include
    ${CMAKE_SOURCE_DIR}/libs/dfsmshsm/include
    ${CMAKE_SOURCE_DIR}/libs/ims-core/include
    ${CMAKE_SOURCE_DIR}/libs/gdg/include
)
target_link_libraries(ims-console-demo PRIVATE
    common
    security
    master-catalog
    vsam
    dfsmshsm
    ims-core
    gdg
    Threads::Threads
)

# =============================================================================
# Add Tests
# =============================================================================

if(IMS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# =============================================================================
# Add Examples
# =============================================================================

if(IMS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# =============================================================================
# Installation
# =============================================================================

install(TARGETS ims-console-demo RUNTIME DESTINATION bin)
install(DIRECTORY libs/common/include/ DESTINATION include)
install(DIRECTORY libs/security/include/ DESTINATION include)
install(DIRECTORY libs/master-catalog/include/ DESTINATION include)
install(DIRECTORY libs/vsam/include/ DESTINATION include)
install(DIRECTORY libs/dfsmshsm/include/ DESTINATION include)
install(DIRECTORY libs/ims-core/include/ DESTINATION include)
install(DIRECTORY libs/gdg/include/ DESTINATION include)
install(DIRECTORY docs/ DESTINATION share/doc/ims-emulation)

message(STATUS "Configuration complete")
message(STATUS "=================================================")
