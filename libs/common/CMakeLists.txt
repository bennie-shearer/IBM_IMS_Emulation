# =============================================================================
# IBM IMS Emulation Enterprise - Common Library
# =============================================================================

add_library(common STATIC
    src/types.cpp
    src/error.cpp
    src/logger.cpp
    src/platform.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(common PUBLIC cxx_std_20)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(common PUBLIC ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(common PUBLIC pthread)
endif()
