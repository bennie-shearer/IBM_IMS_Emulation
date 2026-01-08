# =============================================================================
# IBM IMS Emulation Enterprise - Tests CMakeLists.txt
# Version: 3.6.2
# =============================================================================

# Common include directories
set(TEST_INCLUDES
    ${CMAKE_SOURCE_DIR}/libs/common/include
    ${CMAKE_SOURCE_DIR}/libs/security/include
    ${CMAKE_SOURCE_DIR}/libs/master-catalog/include
    ${CMAKE_SOURCE_DIR}/libs/vsam/include
    ${CMAKE_SOURCE_DIR}/libs/dfsmshsm/include
    ${CMAKE_SOURCE_DIR}/libs/ims-core/include
    ${CMAKE_SOURCE_DIR}/libs/gdg/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Common libraries
set(TEST_LIBS
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
# Common Tests
# =============================================================================
add_executable(common-tests common-tests/test_common.cpp)
target_include_directories(common-tests PRIVATE ${TEST_INCLUDES})
target_link_libraries(common-tests PRIVATE ${TEST_LIBS})
add_test(NAME CommonTests COMMAND common-tests)

# =============================================================================
# Catalog Tests
# =============================================================================
add_executable(catalog-tests catalog-tests/test_catalog.cpp)
target_include_directories(catalog-tests PRIVATE ${TEST_INCLUDES})
target_link_libraries(catalog-tests PRIVATE ${TEST_LIBS})
add_test(NAME CatalogTests COMMAND catalog-tests)

# =============================================================================
# VSAM Tests
# =============================================================================
add_executable(vsam-tests vsam-tests/test_vsam.cpp)
target_include_directories(vsam-tests PRIVATE ${TEST_INCLUDES})
target_link_libraries(vsam-tests PRIVATE ${TEST_LIBS})
add_test(NAME VsamTests COMMAND vsam-tests)

# =============================================================================
# IMS Tests
# =============================================================================
add_executable(ims-tests ims-tests/test_ims.cpp)
target_include_directories(ims-tests PRIVATE ${TEST_INCLUDES})
target_link_libraries(ims-tests PRIVATE ${TEST_LIBS})
add_test(NAME ImsTests COMMAND ims-tests)

# =============================================================================
# New Modules Tests (v3.5.0/v3.6.2)
# =============================================================================
add_executable(new-modules-tests new-modules-tests/test_new_modules.cpp)
target_include_directories(new-modules-tests PRIVATE ${TEST_INCLUDES})
target_link_libraries(new-modules-tests PRIVATE ${TEST_LIBS})
add_test(NAME NewModulesTests COMMAND new-modules-tests)

# =============================================================================
# Benchmark Tests (NEW in v3.6.2)
# =============================================================================
if(IMS_BUILD_BENCHMARKS)
    # Benchmark unit tests
    add_executable(benchmark-tests benchmark-tests/test_benchmark.cpp)
    target_include_directories(benchmark-tests PRIVATE ${TEST_INCLUDES})
    target_link_libraries(benchmark-tests PRIVATE ${TEST_LIBS})
    
    # Standalone benchmark runner
    add_executable(benchmark-runner benchmark-tests/main.cpp)
    target_include_directories(benchmark-runner PRIVATE ${TEST_INCLUDES})
    target_link_libraries(benchmark-runner PRIVATE ${TEST_LIBS})
endif()
