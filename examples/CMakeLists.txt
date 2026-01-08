# =============================================================================
# IBM IMS (Information Management System) Emulation Enterprise - Examples CMakeLists.txt
# Version: 3.6.3
# =============================================================================

# Common include directories for all examples
set(EXAMPLE_INCLUDES
    ${CMAKE_SOURCE_DIR}/libs/common/include
    ${CMAKE_SOURCE_DIR}/libs/security/include
    ${CMAKE_SOURCE_DIR}/libs/master-catalog/include
    ${CMAKE_SOURCE_DIR}/libs/vsam/include
    ${CMAKE_SOURCE_DIR}/libs/dfsmshsm/include
    ${CMAKE_SOURCE_DIR}/libs/ims-core/include
    ${CMAKE_SOURCE_DIR}/libs/gdg/include
)

# Common libraries for all examples
set(EXAMPLE_LIBS
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
# Config Example
# =============================================================================
add_executable(config-example config-example/main.cpp)
target_include_directories(config-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(config-example PRIVATE ${EXAMPLE_LIBS})

# =============================================================================
# Security Example
# =============================================================================
add_executable(security-example security-example/main.cpp)
target_include_directories(security-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(security-example PRIVATE ${EXAMPLE_LIBS})

# =============================================================================
# Catalog Example
# =============================================================================
add_executable(catalog-example catalog-example/main.cpp)
target_include_directories(catalog-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(catalog-example PRIVATE ${EXAMPLE_LIBS})

# =============================================================================
# VSAM Example
# =============================================================================
add_executable(vsam-example vsam-example/main.cpp)
target_include_directories(vsam-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(vsam-example PRIVATE ${EXAMPLE_LIBS})

# =============================================================================
# GDG Example
# =============================================================================
add_executable(gdg-example gdg-example/main.cpp)
target_include_directories(gdg-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(gdg-example PRIVATE ${EXAMPLE_LIBS})

# =============================================================================
# IMS Example
# =============================================================================
add_executable(ims-example ims-example/main.cpp)
target_include_directories(ims-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(ims-example PRIVATE ${EXAMPLE_LIBS})

# =============================================================================
# Batch Example
# =============================================================================
add_executable(batch-example batch-example/main.cpp)
target_include_directories(batch-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(batch-example PRIVATE ${EXAMPLE_LIBS})

# =============================================================================
# Storage Example (NEW in v3.6.3)
# =============================================================================
add_executable(storage-example storage-example/main.cpp)
target_include_directories(storage-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(storage-example PRIVATE ${EXAMPLE_LIBS})

# =============================================================================
# Diagnostic Example (NEW in v3.6.3)
# =============================================================================
add_executable(diagnostic-example diagnostic-example/main.cpp)
target_include_directories(diagnostic-example PRIVATE ${EXAMPLE_INCLUDES})
target_link_libraries(diagnostic-example PRIVATE ${EXAMPLE_LIBS})
