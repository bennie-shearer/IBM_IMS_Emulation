# =============================================================================
# IBM IMS (Information Management System) Emulation Enterprise - Security Library
# =============================================================================

add_library(security STATIC
    src/security_context.cpp
)

target_include_directories(security PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(security PUBLIC common)
target_compile_features(security PUBLIC cxx_std_20)
