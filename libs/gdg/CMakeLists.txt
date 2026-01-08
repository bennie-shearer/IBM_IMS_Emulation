# =============================================================================
# IBM IMS (Information Management System) Emulation Enterprise - GDG Library
# =============================================================================

add_library(gdg STATIC
    src/gdg_manager.cpp
)

target_include_directories(gdg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gdg PUBLIC common master-catalog)
target_compile_features(gdg PUBLIC cxx_std_20)
