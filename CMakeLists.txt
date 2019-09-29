cmake_minimum_required(VERSION 3.0.2)
project(suprafit)
# set the module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)


# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)


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

option (extended_f_test
    "Extend f-test methods for sigma, chi and SEy" OFF)

option (legacy
    "Enable Legacy Concentration Handling - For benchmark purpose (only)" OFF)

option (conservative
    "Enable conservative calculations, slower and not better" OFF)

option (noto_font
    "Enable and Use Google Noto Fonts" OFF)

option (_Theme
    "Use Icon Theme from Desktop Environment, It does not work on Windows" OFF)


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

if(UNIX) # dont work on windows systems, compilation fails unfortranatly ...
    execute_process(
        COMMAND git log -1 --format=%aD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif(UNIX)

add_definitions("-DGIT_COMMIT_HASH=${GIT_COMMIT_HASH}")
add_definitions("-DGIT_BRANCH=${GIT_BRANCH}")
add_definitions("-DGIT_COMMIT_DATE=${GIT_COMMIT_DATE}")

if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    set (_DEBUG ON)
endif(CMAKE_BUILD_TYPE STREQUAL "DEBUG")

if(WIN32) # Check if we are on Windows
    set (_Theme OFF)
endif(WIN32)

if(CMAKE_COMPILER_IS_GNUCXX)
    set (_UNIX  ON)

    find_package(OpenMP)
    if (OPENMP_FOUND)
        set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set (openMP ON)
    endif()
endif()

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-reorder -pedantic \
                                           -Wextra -Wcast-align -Wcast-qual  -Wchar-subscripts  \
                                           -Wcomment -Wdisabled-optimization \
                                           -Wformat  -Wformat=2 -Wformat-nonliteral -Wformat-security\
                                           -Wformat-y2k  -Wimport  -Winit-self  -Winline -Winvalid-pch\
                                           -Wunsafe-loop-optimizations  -Wmissing-braces\
                                           -Wmissing-field-initializers -Wmissing-format-attribute  \
                                           -Wmissing-include-dirs -Wmissing-noreturn -Wpacked  -Wparentheses\
                                           -Wpointer-arith -Wredundant-decls -Wsequence-point\
                                           -Wsign-compare  -Wstack-protector -Wstrict-aliasing\
                                           -Wstrict-aliasing=2 -Wswitch -Wsuggest-override\
                                           -Wtrigraphs  -Wuninitialized -Wunknown-pragmas  -Wunreachable-code\
                                           -Wunused -Wunused-function  -Wunused-label  -Wunused-parameter\
                                           -Wunused-value  -Wunused-variable  -Wvariadic-macros\
                                           -Wvolatile-register-var  -Wwrite-strings -Wdeprecated-declarations\
                                           -Wno-error=unused-local-typedefs -Wno-error=enum-compare -Wno-narrowing -Werror=return-type")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Charts CONFIG REQUIRED)

include_directories(SYSTEM ${QT_INCLUDES} ${PROJECT_SOURCE_DIR}/external/libpeakpick/eigen/)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/external/fisher_dist/ ${PROJECT_SOURCE_DIR}/external/libpeakpick/ ${PROJECT_SOURCE_DIR}/external/CuteChart/src/)

configure_file (
  "${PROJECT_SOURCE_DIR}/src/global_config.h.in"
  "${PROJECT_BINARY_DIR}/src/global_config.h"
)

configure_file(
  "${CMAKE_SOURCE_DIR}/src/version.h.in"
  "${CMAKE_BINARY_DIR}/src/version.h"
)

# include the charts, which where formerly part of SupraFit
add_subdirectory(external/CuteChart)

set(core_SRC
                    src/core/equil.cpp
                    src/core/optimizer/eigen_levenberg.cpp
                    src/core/libmath.cpp
                    src/core/minimizer.cpp
                    src/capabilities/resampleanalyse.cpp
                    src/capabilities/abstractsearchclass.cpp
                    src/capabilities/modelcomparison.cpp
                    src/capabilities/montecarlostatistics.cpp
                    src/capabilities/globalsearch.cpp
                    src/capabilities/weakenedgridsearch.cpp
                    src/capabilities/jobmanager.cpp
                    src/core/analyse.cpp
                    src/core/jsonhandler.cpp
                    src/core/filehandler.cpp
                    src/core/toolset.cpp
                    src/core/instance.h
                    src/global.cpp
)


add_library(core ${core_SRC})

target_link_libraries(core Qt5::Core)

#add_subdirectory(src/core/models/misc)

set(models_SRC
    src/core/models/dataclass.cpp
    src/core/models/AbstractModel.cpp
    src/core/models/AbstractItcModel.cpp
    src/core/models/AbstractTitrationModel.cpp
    src/core/models/meta_model.cpp
    src/core/models/itc/blank.cpp
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
    src/core/models/postprocess/statistic.cpp
    src/core/models/postprocess/thermo.cpp
    src/core/models/misc/indep_quadrat.cpp
    src/core/models/misc/dep_any.cpp
    )

add_library(models ${models_SRC})

target_link_libraries(models Qt5::Core)

set(suprafit_SRCS
    src/ui/guitools/chartwrapper.cpp
    src/ui/guitools/guitools.h
    src/ui/guitools/mime.h
    src/ui/dialogs/regressionanalysisdialog.cpp
    src/ui/dialogs/statisticdialog.cpp
    src/ui/dialogs/modaldialog.cpp
    src/ui/dialogs/advancedsearch.cpp
    src/ui/dialogs/importdata.cpp
    src/ui/dialogs/configdialog.cpp
    src/ui/dialogs/comparedialog.cpp
    src/ui/dialogs/resultsdialog.cpp
    src/ui/dialogs/thermogram.cpp
    src/ui/widgets/metamodelparameter.cpp
    src/ui/widgets/parameterwidget.cpp
    src/ui/widgets/optimizerwidget.cpp
    src/ui/widgets/results/scatterwidget.cpp
    src/ui/widgets/results/searchresultwidget.cpp
    src/ui/widgets/results/mcresultswidget.cpp
    src/ui/widgets/results/resultswidget.cpp
    src/ui/widgets/optionswidget.cpp
    src/ui/widgets/systemparameterwidget.cpp
    src/ui/widgets/modelactions.cpp
    src/ui/widgets/modelchart.cpp
    src/ui/widgets/modelelement.cpp
    src/ui/widgets/signalelement.cpp
    src/ui/widgets/statisticwidget.cpp
    src/ui/widgets/messagedock.cpp
    src/ui/widgets/extwidget.cpp
    src/ui/widgets/buttons/spinbox.h
    src/ui/widgets/buttons/scientificbox.h
    src/ui/widgets/buttons/hovercheckbox.h
    src/ui/widgets/thermogramwidget.cpp
    src/ui/widgets/textwidget.cpp
    src/ui/mainwindow/chartwidget.cpp
    src/ui/mainwindow/modeldataholder.cpp
    src/ui/mainwindow/datawidget.cpp
    src/ui/mainwindow/metamodelwidget.cpp
    src/ui/mainwindow/modelwidget.cpp
    src/ui/mainwindow/mainwindow.cpp
    src/ui/mainwindow/suprafitgui.cpp
    src/ui/main.cpp
)

qt5_add_resources(suprafit_SRCS files.qrc)

if(noto_font)
    qt5_add_resources(suprafit_SRCS fonts.qrc)
endif(noto_font)

# generate proper GUI program on specified platform
if(WIN32) # Check if we are on Windows
    add_executable(suprafit ${suprafit_SRCS} suprafit.rc)

        if(MSVC) # Check if we are using the Visual Studio compiler
                set_target_properties(suprafit PROPERTIES
                        WIN32_EXECUTABLE YES
                        LINK_FLAGS "/ENTRY:mainCRTStartup"
                )
        elseif(CMAKE_COMPILER_IS_GNUCXX)
                         SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Not tested but seems to work
        else()
                message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
        endif(MSVC)
        set(target_dir "bin/win32")
        set_target_properties(suprafit PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )

elseif(APPLE)

    set(SupraFit_icon ${CMAKE_CURRENT_SOURCE_DIR}/misc/SupraFit.icns)
    set_source_files_properties(${SupraFit_icon} PROPERTIES
          MACOSX_PACKAGE_LOCATION "Resources")
    set(MACOSX_BUNDLE_ICON_FILE "SupraFit.icns")

    set(target_dir "bin/macOS")
    add_executable(suprafit MACOSX_BUNDLE ${SupraFit_icon} ${suprafit_SRCS})

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    set(target_dir "bin/linux")

    add_executable(suprafit ${suprafit_SRCS})
    set_target_properties(suprafit PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )

else()
        message(SEND_ERROR "You are on an unsupported platform! (Not Win, Mac OS X or Unix)")
endif(WIN32)

set_target_properties(suprafit PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )



set(suprafit_cli_SRCS
                src/client/suprafit_cli.cpp
                src/client/analyser.cpp
                src/client/simulator.cpp
                src/client/main.cpp
          )

add_executable(suprafit_cli ${suprafit_cli_SRCS})

set_target_properties(suprafit_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )


set_property(TARGET core models  PROPERTY CXX_STANDARD 14)
set_property(TARGET suprafit PROPERTY CXX_STANDARD 14)
set_property(TARGET suprafit_cli PROPERTY CXX_STANDARD 14)

target_link_libraries(suprafit models core cutechart)

target_link_libraries(suprafit_cli models core)

if(testing)
    enable_testing()
    set(basic_test
        test/save_and_load.cpp
    )
    add_test( basic_test supratest )
    add_executable(supratest  ${basic_test})
    #set_property(TARGET suprafit PROPERTY ENABLE_EXPORTS true)
    set_property(TARGET supratest PROPERTY CXX_STANDARD 14)
    target_link_libraries(supratest Qt5::Core Qt5::Test)
    target_link_libraries(supratest models math suprafit ${QT_QTCORE_LIBRARY})
endif(testing)


if(UNIX)
    target_link_libraries(suprafit pthread dl)
    #target_link_libraries(suprafit_cli pthread dl)
endif(UNIX)

# Do platform specific post target stuff
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    install(TARGETS suprafit RUNTIME DESTINATION bin)
    install(TARGETS suprafit_cli RUNTIME DESTINATION bin)
    # Install desktop entry
    install(FILES misc/SupraFit.desktop DESTINATION share/applications )

    # Install icon
    install(FILES misc/SupraFit.png DESTINATION share/icons/pixmaps/)
elseif(APPLE)
    string(REPLACE "qmake" "" qt_dir ${QT_QMAKE_EXECUTABLE})
    add_custom_command(TARGET suprafit POST_BUILD
        COMMAND ${qt_dir}/macdeployqt  suprafit.app -dmg -no-plugins
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${target_dir}
        )
elseif(WIN32)
    #string(REPLACE "qmake" "" qt_dir ${QT_QMAKE_EXECUTABLE})
    #add_custom_command(TARGET suprafit POST_BUILD
    #    COMMAND ${qt_dir}/windeployqt --release   suprafit.app
    #    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${target_dir}
    #    )
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
