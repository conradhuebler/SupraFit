# CMakeLists.txt for SupraFit Tests
# Copyright (C) 2024 Conrad Hübler <Conrad.Huebler@gmx.net>

cmake_minimum_required(VERSION 3.16)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Test Qml)

# Enable testing
enable_testing()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/external/fmt/include)
include_directories(${CMAKE_SOURCE_DIR}/external/libpeakpick/eigen/)
include_directories(${CMAKE_SOURCE_DIR}/external/libpeakpick/)
include_directories(${CMAKE_SOURCE_DIR}/external/CxxThreadPool/include)

# Set up common test libraries - same as suprafit_cli
set(TEST_COMMON_LIBS
    Qt6::Core
    Qt6::Test
    Qt6::Qml
    models
    core
    fmt::fmt-header-only
    ${CMAKE_THREAD_LIBS_INIT}
)

# Add Unix-specific libraries like suprafit_cli
if(UNIX)
    set(TEST_COMMON_LIBS ${TEST_COMMON_LIBS} pthread dl)
endif(UNIX)

# Client sources for test_pipeline (not in libcore/libmodels)
set(CLIENT_SOURCES
    ../client/suprafit_cli.cpp
    ../client/ml_pipeline_manager.cpp
)

# Test for DataTable - link like suprafit_cli
add_executable(test_datatable
    test_datatable.cpp
)

target_link_libraries(test_datatable
    ${TEST_COMMON_LIBS}
)

add_test(NAME DataTableTest COMMAND test_datatable)

# Test for DataClass - link like suprafit_cli
add_executable(test_dataclass
    test_dataclass.cpp
)

target_link_libraries(test_dataclass
    ${TEST_COMMON_LIBS}
)

add_test(NAME DataClassTest COMMAND test_dataclass)

# Test for Pipeline - includes client sources
add_executable(test_pipeline
    test_pipeline.cpp
    ${CLIENT_SOURCES}
)

target_link_libraries(test_pipeline
    ${TEST_COMMON_LIBS}
)

add_test(NAME PipelineTest COMMAND test_pipeline)

# Test for Simple functionality
add_executable(test_simple
    test_simple.cpp
)

target_link_libraries(test_simple
    ${TEST_COMMON_LIBS}
)

add_test(NAME SimpleTest COMMAND test_simple)

# Set test properties
set_tests_properties(DataTableTest PROPERTIES
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(DataClassTest PROPERTIES
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_tests_properties(PipelineTest PROPERTIES
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add simple test for basic functionality
add_executable(test_datatable_simple
    test_datatable_simple.cpp
)

target_link_libraries(test_datatable_simple
    Qt6::Core
    Qt6::Test
)

add_test(NAME DataTableSimpleTest COMMAND test_datatable_simple)

# Test for DataGenerator functionality - Claude Generated
add_executable(test_datagenerator
    test_datagenerator.cpp
)

target_link_libraries(test_datagenerator
    ${TEST_COMMON_LIBS}
)

add_test(NAME DataGeneratorTest COMMAND test_datagenerator)

set_tests_properties(DataGeneratorTest PROPERTIES
    TIMEOUT 30
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_datatable test_dataclass test_pipeline test_simple test_datatable_simple test_datagenerator
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add custom target for test coverage (if available)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(GCOV_PATH gcov)
    if(GCOV_PATH)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --verbose
            COMMAND gcov -r ${CMAKE_BINARY_DIR}/src/tests/*.gcno
            DEPENDS test_datatable test_dataclass test_pipeline
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()
endif()

# Create test data directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data)

# Install test data files (if any)
# install(FILES test_data.dat DESTINATION ${CMAKE_BINARY_DIR}/test_data)

message(STATUS "Test configuration complete")
message(STATUS "Run tests with: make run_tests")
message(STATUS "Individual tests:")
message(STATUS "  DataTable: ./test_datatable")
message(STATUS "  DataClass: ./test_dataclass")
message(STATUS "  Pipeline:  ./test_pipeline")