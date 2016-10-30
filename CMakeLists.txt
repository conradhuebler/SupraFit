cmake_minimum_required(VERSION 3.0.2)
project(nmr2fit)
# set the module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option (BENCHMARK_LEVEL
    "Benchmark output enabled" OFF)

configure_file (
  "${PROJECT_SOURCE_DIR}/global_config.h.in"
  "${PROJECT_BINARY_DIR}/global_config.h"
)
  
IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wno-reorder")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)


set(CMAKE_C_FLAGS "-I/usr/include/levmar")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(nmr2fit_SRCS
                core/filehandler.cpp
                core/libmath.cpp
                core/data/dataclass.cpp
                ui/dialogs/importdata.cpp
                ui/dialogs/configdialog.cpp
                ui/dialogs/chartconfig.cpp
                core/data/modelclass.cpp
                ui/widgets/modeldataholder.cpp
                ui/widgets/optimizerwidget.cpp
                ui/widgets/chartwidget.cpp
                ui/widgets/chartview.cpp
                ui/widgets/datawidget.cpp
                ui/widgets/modelwidget.cpp
                ui/nmr2fit.cpp 
                global.cpp
                main.cpp)
add_executable(nmr2fit ${nmr2fit_SRCS})

set_property(TARGET nmr2fit PROPERTY CXX_STANDARD 11)

qt5_use_modules(nmr2fit Widgets Network Charts PrintSupport)

target_link_libraries(nmr2fit ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTPRINT_LIBRARY} Qt5::PrintSupport levmar)

install(TARGETS nmr2fit RUNTIME DESTINATION bin)
