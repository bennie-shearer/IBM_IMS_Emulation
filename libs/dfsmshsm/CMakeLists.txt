# =============================================================================
# IBM IMS (Information Management System) Emulation Enterprise - DFSMShsm Library
# =============================================================================

add_library(dfsmshsm STATIC
    src/storage_manager.cpp
)

target_include_directories(dfsmshsm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(dfsmshsm PUBLIC common master-catalog security)
target_compile_features(dfsmshsm PUBLIC cxx_std_20)
