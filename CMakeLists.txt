cmake_minimum_required(VERSION 3.0.2)
project(suprafit)
# set the module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

option (BENCHMARK_LEVEL
    "Benchmark output enabled" OFF)
    
option (experimental
    "Include experimental functions" OFF)
 
option (testing
    "Enable unittests" OFF)
 

# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions("-DGIT_COMMIT_HASH=${GIT_COMMIT_HASH}")
add_definitions("-DGIT_BRANCH=${GIT_BRANCH}")

configure_file (
  "${PROJECT_SOURCE_DIR}/src/global_config.h.in"
  "${PROJECT_BINARY_DIR}/src/global_config.h"
)

configure_file(
  "${CMAKE_SOURCE_DIR}/src/version.h.in"
  "${CMAKE_BINARY_DIR}/src/version.h"
)


# generate proper GUI program on specified platform
if(WIN32) # Check if we are on Windows
	if(MSVC) # Check if we are using the Visual Studio compiler
		set_target_properties(${PROJECT_NAME} PROPERTIES
			WIN32_EXECUTABLE YES
			LINK_FLAGS "/ENTRY:mainCRTStartup"
		)
	elseif(CMAKE_COMPILER_IS_GNUCXX)
			 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Not tested but seems to work
	else()
		message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
	endif(MSVC)
elseif(APPLE)
	set_target_properties(${PROJECT_NAME} PROPERTIES
			MACOSX_BUNDLE YES
	)
elseif(UNIX)
	# Nothing special required
else()
	message(SEND_ERROR "You are on an unsupported platform! (Not Win32, Mac OS X or Unix)")
endif(WIN32)

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
                                           -Wstrict-aliasing=2 -Wswitch \
                                           -Wtrigraphs  -Wuninitialized -Wunknown-pragmas  -Wunreachable-code\
                                           -Wunused -Wunused-function  -Wunused-label  -Wunused-parameter\
                                           -Wunused-value  -Wunused-variable  -Wvariadic-macros\
                                           -Wvolatile-register-var  -Wwrite-strings -Wno-deprecated-declarations\
                                           -Wno-error=unused-local-typedefs -Wno-error=enum-compare")                                        
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5Charts REQUIRED)
find_package(Qt5DataVisualization REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} eigen/)

if(experimental)
    include_directories(ChaiScript/include)
endif(experimental)

set(libmath_SRC
                src/core/equal_system.cpp
                src/core/optimizer/eigen_levenberg.cpp
                src/core/libmath.cpp
                src/core/minimizer.cpp
)
if(testing)
    add_library(math ${libmath_SRC})
else()
    add_library(math STATIC ${libmath_SRC})
endif(testing)
qt5_use_modules(math Core)

set(models_SRC
                src/core/dataclass.cpp
                src/core/AbstractModel.cpp
                src/core/models/1_1_Model.cpp
                src/core/models/2_1_1_1_Model.cpp
                src/core/models/1_1_1_2_Model.cpp
                src/core/models/2_1_1_1_1_2_Model.cpp
                src/core/models/ScriptModel.cpp
)

if(testing)
    add_library(models ${models_SRC})
else()
    add_library(models STATIC ${models_SRC})
endif(testing)

qt5_use_modules(models Core)
set(utils_SRC
                src/capabilities/montecarlostatistics.cpp
                src/capabilities/globalsearch.cpp
                src/capabilities/continuousvariation.cpp
                src/core/jsonhandler.cpp
                src/core/filehandler.cpp
                src/core/toolset.cpp
                src/global.cpp
)
if(testing)
    add_library(utils ${utils_SRC})
else()
    add_library(utils STATIC ${utils_SRC})
endif(testing)

qt5_use_modules(utils Core Gui)

set(suprafit_SRCS
                src/ui/guitools/chartwrapper.cpp
                src/ui/widgets/modelactions.cpp
                src/ui/widgets/modelelement.cpp
                src/ui/widgets/signalelement.cpp
                src/ui/widgets/statisticwidget.cpp
                src/ui/widgets/modeltablewidget.cpp
                src/ui/widgets/optimizerflagwidget.cpp
                src/ui/widgets/3dchartview.cpp
                src/ui/widgets/chartview.cpp
                src/ui/widgets/buttons/spinbox.h
                src/ui/widgets/buttons/scientificbox.h
                src/ui/dialogs/statisticdialog.cpp
                src/ui/dialogs/modeldialog.cpp
                src/ui/dialogs/advancedsearch.cpp
                src/ui/dialogs/importdata.cpp
                src/ui/dialogs/configdialog.cpp
                src/ui/dialogs/chartconfig.cpp
                src/ui/widgets/optimizerwidget.cpp
                src/ui/mainwindow/modelhistorywidget.cpp
                src/ui/mainwindow/chartwidget.cpp
                src/ui/mainwindow/modeldataholder.cpp
                src/ui/mainwindow/datawidget.cpp
                src/ui/mainwindow/modelwidget.cpp
                src/ui/mainwindow/suprafit.cpp 
                src/main.cpp)
qt5_add_resources(suprafit_SRCS files.qrc)
add_executable(suprafit ${suprafit_SRCS})


set_property(TARGET utils math models suprafit PROPERTY CXX_STANDARD 14)

qt5_use_modules(suprafit Widgets Charts PrintSupport DataVisualization)
target_link_libraries(suprafit  models math  utils ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTPRINT_LIBRARY} Qt5::PrintSupport)

if(testing)
    enable_testing()
    set(test_additional
        test/testjson.cpp
    )
    add_test( test_additional json_test )
    add_executable(json_test  ${test_additional})
    set_property(TARGET suprafit PROPERTY ENABLE_EXPORTS true)
    set_property(TARGET  json_test PROPERTY CXX_STANDARD 14)
    qt5_use_modules(json_test Core Test)
    target_link_libraries(json_test utils models math suprafit ${QT_QTCORE_LIBRARY})
endif(testing)


if(UNIX)
    target_link_libraries(suprafit pthread dl)
endif(UNIX)

install(TARGETS suprafit RUNTIME DESTINATION bin)
