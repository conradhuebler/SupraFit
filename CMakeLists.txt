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
  "${PROJECT_SOURCE_DIR}/src/global_config.h.in"
  "${PROJECT_BINARY_DIR}/src/global_config.h"
)

IF(CMAKE_COMPILER_IS_GNUCXX)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-reorder -pedantic-errors \
                                           -Wextra -Wcast-align -Wcast-qual  -Wchar-subscripts  \
                                           -Wcomment -Wdisabled-optimization \
                                           -Wformat  -Wformat=2 -Wformat-nonliteral -Wformat-security\
                                           -Wformat-y2k  -Wimport  -Winit-self  -Winline -Winvalid-pch\
                                           -Wunsafe-loop-optimizations  -Wmissing-braces\
                                           -Wmissing-field-initializers -Wmissing-format-attribute  \
                                           -Wmissing-include-dirs -Wmissing-noreturn -Wpacked  -Wparentheses\
                                           -Wpointer-arith -Wredundant-decls -Wreturn-type -Wsequence-point\
                                           -Wsign-compare  -Wstack-protector -Wstrict-aliasing\
                                           -Wstrict-aliasing=2 -Wswitch  -Wswitch-default -Wswitch-enum\
                                           -Wtrigraphs  -Wuninitialized -Wunknown-pragmas  -Wunreachable-code\
                                           -Wunused -Wunused-function  -Wunused-label  -Wunused-parameter\
                                           -Wunused-value  -Wunused-variable  -Wvariadic-macros\
                                           -Wvolatile-register-var  -Wwrite-strings -Wno-deprecated-declarations\
                                           -Wno-error=unused-local-typedefs -Wno-error=enum-compare")                                        
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
option(USE_levmarOptimizer "Use levmar optimizing package" OFF)  
option(USE_eigenOptimizer  "Use Eigen optimizing package" OFF)  
option(USE_bfgsOptimizer  "Use Eigen based bfgs optimizing package" OFF) 

if(USE_levmarOptimizer)
    if(USE_eigenOptimizer)
            message( FATAL_ERROR "Can't use levmar and eigen together.")
    endif(USE_eigenOptimizer)
endif(USE_levmarOptimizer)

if(USE_eigenOptimizer)
    if(USE_levmarOptimizer)
            message( FATAL_ERROR "Can't use levmar and eigen together.")
    endif(USE_levmarOptimizer)
endif(USE_eigenOptimizer)

if(NOT USE_eigenOptimizer)    
    if(NOT USE_levmarOptimizer)
                set(USE_bfgsOptimizer ON)
    endif(NOT USE_levmarOptimizer)
endif(NOT USE_eigenOptimizer)

if(USE_levmarOptimizer)
    set(CMAKE_C_FLAGS "-I/usr/include/levmar")
endif(USE_levmarOptimizer)


find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} Eigen/)


set(libmath_SRC
                src/core/optimizer/levmar.cpp
                src/core/optimizer/eigen_levenberg.cpp
                src/core/optimizer/bfgs.cpp
                src/core/libmath.cpp
)
add_library(math STATIC ${libmath_SRC})
qt5_use_modules(math Widgets)

set(models_SRC
                src/core/dataclass.cpp
                src/core/AbstractModel.cpp
                src/core/models/1_1_Model.cpp
                src/core/models/2_1_1_1_Model.cpp
                src/core/models/1_1_1_2_Model.cpp
)
add_library(models STATIC ${models_SRC})
qt5_use_modules(models Widgets Charts)

set(nmr2fit_SRCS
                src/ui/widgets/modelhistorywidget.cpp
                src/ui/dialogs/importdata.cpp
                src/ui/dialogs/configdialog.cpp
                src/ui/dialogs/chartconfig.cpp
                src/ui/widgets/modeldataholder.cpp
                src/ui/widgets/optimizerwidget.cpp
                src/ui/widgets/chartwidget.cpp
                src/ui/widgets/chartview.cpp
                src/ui/widgets/datawidget.cpp
                src/ui/widgets/modelwidget.cpp
                src/ui/nmr2fit.cpp 
                src/core/jsonhandler.cpp
                src/core/filehandler.cpp
                src/core/toolset.cpp
                src/global.cpp
                src/main.cpp)

add_executable(nmr2fit ${nmr2fit_SRCS})

set_property(TARGET math models nmr2fit PROPERTY CXX_STANDARD 11)

qt5_use_modules(nmr2fit Widgets Charts PrintSupport)

target_link_libraries(nmr2fit models math  ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTPRINT_LIBRARY} Qt5::PrintSupport)

if(USE_levmarOptimizer)
    target_link_libraries(nmr2fit levmar)
endif(USE_levmarOptimizer)

install(TARGETS nmr2fit RUNTIME DESTINATION bin)
