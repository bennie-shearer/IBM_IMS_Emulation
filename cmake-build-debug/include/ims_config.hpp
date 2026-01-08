#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - Generated Configuration Header
// Version: 3.6.2
// =============================================================================

#define IMS_VERSION_MAJOR 3
#define IMS_VERSION_MINOR 6
#define IMS_VERSION_PATCH 2
#define IMS_VERSION_STRING "3.6.2"

// Platform Detection
#define IMS_PLATFORM_WINDOWS
/* #undef IMS_PLATFORM_LINUX */
/* #undef IMS_PLATFORM_MACOS */

// Build Configuration
/* #undef IMS_ENABLE_OPENSSL */
#define IMS_BUILD_TESTS

// CPU Features
#define IMS_HAS_SSE42
#define IMS_HAS_AVX2

// Build Type
#define CMAKE_BUILD_TYPE "Debug"
