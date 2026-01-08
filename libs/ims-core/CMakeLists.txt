# =============================================================================
# IBM IMS Emulation Enterprise - IMS Core Library
# =============================================================================

add_library(ims-core STATIC
    src/ims_database.cpp
)

target_include_directories(ims-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ims-core PUBLIC common vsam security master-catalog)
target_compile_features(ims-core PUBLIC cxx_std_20)
