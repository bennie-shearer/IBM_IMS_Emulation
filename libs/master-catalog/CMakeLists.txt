# =============================================================================
# IBM IMS (Information Management System) Emulation Enterprise - Master Catalog Library
# =============================================================================

add_library(master-catalog STATIC
    src/master_catalog.cpp
)

target_include_directories(master-catalog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(master-catalog PUBLIC common security)
target_compile_features(master-catalog PUBLIC cxx_std_20)
