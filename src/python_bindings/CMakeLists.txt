# CMake configuration for SupraFit Python Bindings
# Created by Claude Code AI Assistant

cmake_minimum_required(VERSION 3.21.0)

# Find Python and pybind11
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Try to find pybind11 - if not found, download it
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found, downloading...")
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.11.1
    )
    FetchContent_MakeAvailable(pybind11)
endif()

# Source files for Python bindings
set(PYTHON_BINDING_SOURCES
    bindings_main.cpp
    py_dataclass.cpp
    py_models.cpp
    py_statistics.cpp
    py_io.cpp
)

# Create Python module (named _suprafit to avoid conflict with GUI executable)
pybind11_add_module(_suprafit ${PYTHON_BINDING_SOURCES})

# Link against SupraFit libraries
target_link_libraries(_suprafit PRIVATE
    models
    core
    Qt::Core
    Qt::Qml
    fmt::fmt-header-only
)

# Include directories
target_include_directories(_suprafit PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/external/libpeakpick/eigen/
    ${PROJECT_SOURCE_DIR}/external/fmt/include
)

# Set C++ standard
set_property(TARGET _suprafit PROPERTY CXX_STANDARD 17)

# Installation
install(TARGETS _suprafit
    LIBRARY DESTINATION ${Python3_SITELIB}
    COMPONENT python
)

# Optional: Copy to build directory for testing
add_custom_command(TARGET _suprafit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:_suprafit> ${CMAKE_BINARY_DIR}/
    COMMENT "Copying Python module to build directory"
)

# Create Python package wrapper that imports _suprafit
# This allows users to do: import suprafit as sf
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/suprafit/__init__.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/suprafit/__init__.py"
    @ONLY
)

# Install the wrapper package
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/suprafit/__init__.py"
    DESTINATION ${Python3_SITELIB}/suprafit
    COMPONENT python
)

message(STATUS "Python bindings configured:")
message(STATUS "  Python version: ${Python3_VERSION}")
message(STATUS "  Python executable: ${Python3_EXECUTABLE}")
message(STATUS "  Install location: ${Python3_SITELIB}")
message(STATUS "  Python module: _suprafit (C++ extension)")
message(STATUS "  Python package: suprafit (wrapper)")
