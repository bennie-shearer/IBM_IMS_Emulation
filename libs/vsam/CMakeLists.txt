# =============================================================================
# IBM IMS Emulation Enterprise - VSAM Library
# =============================================================================

add_library(vsam STATIC
    src/vsam_types.cpp
)

target_include_directories(vsam PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(vsam PUBLIC common master-catalog)
target_compile_features(vsam PUBLIC cxx_std_20)
