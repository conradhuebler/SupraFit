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
 
option (NMR_Models
    "Enable NMR Titration models" ON)
    
option (ITC_Models
    "Enable ITC Titration models" ON)
    
option (Kinetic_Models
    "Enable Kinetic Models" ON)
    
option (Fluorescence_Models
    "Enable Fluorescence Models" ON)
 
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

execute_process(
  COMMAND git log -1 --format=%aD
   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_DATE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions("-DGIT_COMMIT_HASH=${GIT_COMMIT_HASH}")
add_definitions("-DGIT_BRANCH=${GIT_BRANCH}")
add_definitions("-DGIT_COMMIT_DATE=${GIT_COMMIT_DATE}")

if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    set (_DEBUG ON)
endif(CMAKE_BUILD_TYPE STREQUAL "DEBUG")


find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

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
	set (_Theme ON)
else()
	message(SEND_ERROR "You are on an unsupported platform! (Not Win32, Mac OS X or Unix)")
endif(WIN32)

configure_file (
  "${PROJECT_SOURCE_DIR}/src/global_config.h.in"
  "${PROJECT_BINARY_DIR}/src/global_config.h"
)

configure_file(
  "${CMAKE_SOURCE_DIR}/src/version.h.in"
  "${CMAKE_BINARY_DIR}/src/version.h"
)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-reorder -pedantic \
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

include_directories(SYSTEM ${QT_INCLUDES} eigen/)
include_directories(${CMAKE_CURRENT_BINARY_DIR} fisher_dist/ libpeakpick/)

if(experimental)
    include_directories(SYSTEM ChaiScript/include)
endif(experimental)

set(libmath_SRC
                src/core/equal_system.cpp
                src/core/equil.cpp
                src/core/optimizer/eigen_levenberg.cpp
                src/core/libmath.cpp
                src/core/minimizer.cpp
)


add_library(math ${libmath_SRC})

qt5_use_modules(math Core)

set(models_SRC
                src/core/dataclass.cpp
                src/core/AbstractModel.cpp
                src/core/AbstractItcModel.cpp
                src/core/AbstractTitrationModel.cpp
                src/core/models/itc/itc_1_1_Model.cpp
                src/core/models/itc/itc_2_1_Model.cpp
                src/core/models/itc/itc_1_2_Model.cpp
                src/core/models/itc/itc_2_2_Model.cpp
                src/core/models/itc/itc_n_1_1_Model.cpp
                src/core/models/itc/itc_n_1_2_Model.cpp
                src/core/models/fluorescence/fl_1_1_Model.cpp
                src/core/models/fluorescence/fl_1_1_1_2_Model.cpp
                src/core/models/fluorescence/fl_2_1_1_1_Model.cpp
                src/core/models/fluorescence/fl_2_1_1_1_1_2_Model.cpp
                src/core/models/titrations/1_1_Model.cpp
                src/core/models/titrations/2_1_1_1_Model.cpp
                src/core/models/titrations/1_1_1_2_Model.cpp
                src/core/models/titrations/2_1_1_1_1_2_Model.cpp
                src/core/models/titrations/ScriptModel.cpp
                src/core/models/kinetics/mm_model.cpp
                src/core/models/kinetics/monomolecularmodel.cpp
)

add_library(models ${models_SRC})

qt5_use_modules(models Core)
set(utils_SRC
                src/capabilities/reductionanalyse.cpp
                src/capabilities/abstractsearchclass.cpp
                src/capabilities/modelcomparison.cpp
                src/capabilities/montecarlostatistics.cpp
                src/capabilities/globalsearch.cpp
                src/capabilities/weakenedgridsearch.cpp
                src/core/analyse.cpp
                src/core/jsonhandler.cpp
                src/core/filehandler.cpp
                src/core/toolset.cpp
                src/global.cpp
)

add_library(utils ${utils_SRC})

qt5_use_modules(utils Core)

set(suprafit_SRCS
                src/ui/guitools/chartwrapper.cpp
                src/ui/guitools/instance.h
                src/ui/dialogs/regressionanalysisdialog.cpp
                src/ui/dialogs/statisticdialog.cpp
                src/ui/dialogs/modeldialog.cpp
                src/ui/dialogs/advancedsearch.cpp
                src/ui/dialogs/importdata.cpp
                src/ui/dialogs/configdialog.cpp
                src/ui/dialogs/comparedialog.cpp
                src/ui/dialogs/chartconfig.cpp
                src/ui/dialogs/thermogram.cpp
                src/ui/widgets/parameterwidget.cpp
                src/ui/widgets/optimizerwidget.cpp
                src/ui/widgets/results/contourwidget.cpp
                src/ui/widgets/results/searchresultwidget.cpp
                src/ui/widgets/results/mcresultswidget.cpp
                src/ui/widgets/results/resultswidget.cpp
                src/ui/widgets/optionswidget.cpp
                src/ui/widgets/systemparameterwidget.cpp
                src/ui/widgets/modelactions.cpp
                src/ui/widgets/modelelement.cpp
                src/ui/widgets/signalelement.cpp
                src/ui/widgets/statisticwidget.cpp
                src/ui/widgets/optimizerflagwidget.cpp
                src/ui/widgets/chartview.cpp
                src/ui/widgets/listchart.cpp
                src/ui/widgets/buttons/spinbox.h
                src/ui/widgets/buttons/scientificbox.h
                src/ui/widgets/buttons/hovercheckbox.h
                src/ui/mainwindow/modelhistorywidget.cpp
                src/ui/mainwindow/chartwidget.cpp
                src/ui/mainwindow/modeldataholder.cpp
                src/ui/mainwindow/datawidget.cpp
                src/ui/mainwindow/modelwidget.cpp
                src/ui/mainwindow/suprafit.cpp 
                src/main.cpp)
qt5_add_resources(suprafit_SRCS files.qrc)
add_executable(suprafit ${suprafit_SRCS})


set(suprafit_cli_SRCS
                src/core/minimizer.cpp # whatsoever ...
                src/client/console.cpp
                src/client/main.cpp
          )
          
add_executable(suprafit_cli ${suprafit_cli_SRCS})


set_property(TARGET math models utils PROPERTY CXX_STANDARD 14)
set_property(TARGET suprafit PROPERTY CXX_STANDARD 14)
set_property(TARGET suprafit_cli PROPERTY CXX_STANDARD 14)

qt5_use_modules(suprafit Core Widgets Charts PrintSupport)
qt5_use_modules(suprafit_cli Core)

target_link_libraries(suprafit models math utils ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTPRINT_LIBRARY} Qt5::PrintSupport)

target_link_libraries(suprafit_cli models math utils ${QT_QTCORE_LIBRARY} )

if(testing)
    enable_testing()
    set(basic_test
        test/save_and_load.cpp
    )
    add_test( basic_test supratest )
    add_executable(supratest  ${basic_test})
    #set_property(TARGET suprafit PROPERTY ENABLE_EXPORTS true)
    set_property(TARGET supratest PROPERTY CXX_STANDARD 14)
    qt5_use_modules(supratest Core Test)
    target_link_libraries(supratest utils models math suprafit ${QT_QTCORE_LIBRARY})
endif(testing)


if(UNIX)
    target_link_libraries(suprafit pthread dl)
    target_link_libraries(suprafit_cli pthread dl)
endif(UNIX)

install(TARGETS suprafit RUNTIME DESTINATION bin)
install(TARGETS suprafit_cli RUNTIME DESTINATION bin)
