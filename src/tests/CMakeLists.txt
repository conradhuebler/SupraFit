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

# Comprehensive CLI Tests - Claude Generated 2025-09-02
# These tests validate all active SupraFit CLI functionality

# CLI Core Tests - Basic argument parsing and configuration loading
add_executable(test_cli_core
    test_cli_core.cpp
)

target_link_libraries(test_cli_core
    ${TEST_COMMON_LIBS}
)

add_test(NAME CliCoreTest COMMAND test_cli_core)

set_tests_properties(CliCoreTest PROPERTIES
    TIMEOUT 60
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Data Generation Tests - Modular structure and equation-based generation
add_executable(test_data_generation
    test_data_generation.cpp
)

target_link_libraries(test_data_generation
    ${TEST_COMMON_LIBS}
)

add_test(NAME DataGenerationTest COMMAND test_data_generation)

set_tests_properties(DataGenerationTest PROPERTIES
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Model Fitting Tests - AddModels structure and parameter fitting
add_executable(test_model_fitting
    test_model_fitting.cpp
)

target_link_libraries(test_model_fitting
    ${TEST_COMMON_LIBS}
)

add_test(NAME ModelFittingTest COMMAND test_model_fitting)

set_tests_properties(ModelFittingTest PROPERTIES
    TIMEOUT 180
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Post-Processing Tests - All 7 statistical analysis methods
add_executable(test_post_processing
    test_post_processing.cpp
)

target_link_libraries(test_post_processing
    ${TEST_COMMON_LIBS}
)

add_test(NAME PostProcessingTest COMMAND test_post_processing)

set_tests_properties(PostProcessingTest PROPERTIES
    TIMEOUT 240
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ML Feature Extraction Tests - Compact feature generation and JsonUtils
add_executable(test_ml_extraction
    test_ml_extraction.cpp
)

target_link_libraries(test_ml_extraction
    ${TEST_COMMON_LIBS}
)

add_test(NAME MLExtractionTest COMMAND test_ml_extraction)

set_tests_properties(MLExtractionTest PROPERTIES
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Multi-Project Tests - Project extraction, UUID/title matching, file joining
add_executable(test_multi_project
    test_multi_project.cpp
)

target_link_libraries(test_multi_project
    ${TEST_COMMON_LIBS}
)

add_test(NAME MultiProjectTest COMMAND test_multi_project)

set_tests_properties(MultiProjectTest PROPERTIES
    TIMEOUT 90
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# File I/O Tests - Format conversion and file structure analysis
add_executable(test_file_operations
    test_file_operations.cpp
)

target_link_libraries(test_file_operations
    ${TEST_COMMON_LIBS}
)

add_test(NAME FileOperationsTest COMMAND test_file_operations)

set_tests_properties(FileOperationsTest PROPERTIES
    TIMEOUT 90
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Integration Tests - End-to-end workflows and system validation
add_executable(test_integration
    test_integration.cpp
)

target_link_libraries(test_integration
    ${TEST_COMMON_LIBS}
)

add_test(NAME IntegrationTest COMMAND test_integration)

set_tests_properties(IntegrationTest PROPERTIES
    TIMEOUT 600  # 10 minutes for comprehensive integration tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Comprehensive Real Data Test - Complete workflow with 4 models × 5 analyses (Claude Generated)
add_executable(test_comprehensive_real_data
    test_comprehensive_real_data.cpp
)

target_link_libraries(test_comprehensive_real_data
    ${TEST_COMMON_LIBS}
)

add_test(NAME ComprehensiveRealDataTest COMMAND test_comprehensive_real_data)

set_tests_properties(ComprehensiveRealDataTest PROPERTIES
    TIMEOUT 1800  # 30 minutes for comprehensive real data testing
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add custom target to run all tests including new comprehensive CLI tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS test_datatable test_dataclass test_pipeline test_simple test_datatable_simple test_datagenerator
            test_cli_core test_data_generation test_model_fitting test_post_processing
            test_ml_extraction test_multi_project test_file_operations test_integration
            test_comprehensive_real_data
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add custom target to run only the comprehensive CLI tests
add_custom_target(run_cli_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose -R "(CliCore|DataGeneration|ModelFitting|PostProcessing|MLExtraction|MultiProject|FileOperations|Integration|ComprehensiveRealData)Test"
    DEPENDS test_cli_core test_data_generation test_model_fitting test_post_processing
            test_ml_extraction test_multi_project test_file_operations test_integration
            test_comprehensive_real_data
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
message(STATUS "Run all tests with: make run_tests")
message(STATUS "Run CLI tests only: make run_cli_tests")
message(STATUS "")
message(STATUS "Individual existing tests:")
message(STATUS "  DataTable: ./test_datatable")
message(STATUS "  DataClass: ./test_dataclass")
message(STATUS "  Pipeline:  ./test_pipeline")
message(STATUS "  Simple:    ./test_simple")
message(STATUS "  DataGen:   ./test_datagenerator")
message(STATUS "")
message(STATUS "Comprehensive CLI tests (Claude Generated 2025-09-02):")
message(STATUS "  CLI Core:           ./test_cli_core")
message(STATUS "  Data Generation:    ./test_data_generation")
message(STATUS "  Model Fitting:      ./test_model_fitting")
message(STATUS "  Post-Processing:    ./test_post_processing")
message(STATUS "  ML Extraction:      ./test_ml_extraction")
message(STATUS "  Multi-Project:      ./test_multi_project")
message(STATUS "  File Operations:    ./test_file_operations")
message(STATUS "  Integration:        ./test_integration")
message(STATUS "  Real Data Analysis: ./test_comprehensive_real_data")