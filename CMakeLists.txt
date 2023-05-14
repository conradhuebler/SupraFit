cmake_minimum_required(VERSION 3.21.0)
project(SupraFit
    VERSION 2.6.0
    LANGUAGES CXX)
# set the module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)


# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake to run moc when necessary:

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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

option (_Python
    "Enable python interface" OFF)

option (_Models
	"Enable custom models" ON)

option (Use_Duktape
    "Use Duktape script engine" OFF )

# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the latest tag within the working branch
execute_process(
  COMMAND git describe --tags
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

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
add_definitions("-DGIT_COMMIT_TAG=${GIT_COMMIT_TAG}")

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

find_package(Qt6 6.2 COMPONENTS Core Widgets Charts Qml)

if(_Python)
    find_package(PythonLibs)
    if( NOT PythonLibs_FOUND)
        message(FATAL_ERROR "Python is not found on your system. Please rerun cmake with disabled Python support!")
    endif()
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/external/fmt EXCLUDE_FROM_ALL)


include_directories(SYSTEM ${QT_INCLUDES} ${PROJECT_SOURCE_DIR}/external/fmt/include ${PROJECT_SOURCE_DIR}/external/libpeakpick/eigen/ ${PYTHON_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/external/ChaiScript/include/ ${PROJECT_SOURCE_DIR}/external/ChaiScript_Extras/include/)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external/fisher_dist/ ${PROJECT_SOURCE_DIR}/external/libpeakpick/ ${PROJECT_SOURCE_DIR}/external/CuteChart/src/ ${PROJECT_SOURCE_DIR}/external/CxxThreadPool/include ${PROJECT_SOURCE_DIR}/external/EqnConc/include)
#${PROJECT_SOURCE_DIR}/external/CxxClust/ )

configure_file (
  "${PROJECT_SOURCE_DIR}/src/global_config.h.in"
  "${PROJECT_BINARY_DIR}/src/global_config.h"
)

configure_file(
  "${CMAKE_SOURCE_DIR}/src/version.h.in"
  "${CMAKE_BINARY_DIR}/src/version.h"
)

if(Use_Duktape)
    include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/external/duktape/src)
endif()

# include the charts, which where formerly part of SupraFit
add_subdirectory(external/CuteChart)

set(core_SRC
    src/core/equil.cpp
    src/core/libmath.cpp
    src/core/minimizer.cpp
    src/core/optimizer/eigen_levenberg.cpp
    src/capabilities/datagenerator.cpp
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
    src/core/thermogramhandler.cpp
    src/core/spectrahandler.cpp
    src/core/concentrationalpolynomial.cpp
    src/global.cpp
)

IF(_Python)
    set(python_SRC
        src/core/pythonbridge.cpp
        )
ENDIF(_Python)

if(_Python)
    add_library(core SHARED ${core_SRC})
else()
    add_library(core ${core_SRC})
endif()

target_link_libraries(core Qt::Core Qt::Qml)

#add_subdirectory(src/core/models/misc)
set(models_SRC
    src/core/models/dataclass.cpp
    src/core/models/datatable.cpp
    src/core/models/AbstractModel.cpp
    src/core/models/titrations/AbstractItcModel.cpp
    src/core/models/titrations/AbstractTitrationModel.cpp
    src/core/models/titrations/AbstractNMRModel.cpp

    src/core/models/meta_model.cpp
    src/core/models/scriptmodel.cpp
    src/core/models/pymodelinterpreter.cpp
    src/core/models/chaiinterpreter.cpp
    src/core/models/kinetics/mm_model.cpp
    src/core/models/kinetics/monomolecularmodel.cpp
    src/core/models/kinetics/bimolecularmodel.cpp
    src/core/models/kinetics/flexmolecularmodel.cpp
    src/core/models/kinetics/tian.cpp
    src/core/models/kinetics/evap.cpp

    src/core/models/postprocess/statistic.cpp
    src/core/models/postprocess/thermo.cpp
    src/core/models/misc/indep_quadrat.cpp
    src/core/models/misc/dep_any.cpp
    src/core/models/photophysics/decayrates.cpp
    src/core/models/thermodynamics/eyring.cpp
    src/core/models/thermodynamics/arrhenius.cpp
    src/core/models/thermodynamics/bet.cpp

    src/core/models/titrations/fluorescence/fl_1_1_1_2_Model.cpp
    src/core/models/titrations/fluorescence/fl_1_1_Model.cpp
    src/core/models/titrations/fluorescence/fl_2_1_1_1_1_2_Model.cpp
    src/core/models/titrations/fluorescence/fl_2_1_1_1_Model.cpp

    src/core/models/titrations/itc/blank.cpp
    src/core/models/titrations/itc/itc_1_1_Model.cpp
    src/core/models/titrations/itc/itc_1_2_Model.cpp
    src/core/models/titrations/itc/itc_2_1_Model.cpp
    src/core/models/titrations/itc/itc_2_2_Model.cpp
    src/core/models/titrations/itc/itc_n_1_1_Model.cpp
    src/core/models/titrations/itc/itc_n_1_2_Model.cpp
    src/core/models/titrations/itc/itc_any_Model.cpp

    src/core/models/titrations/nmr/nmr_1_1_1_2_Model.cpp
    src/core/models/titrations/nmr/nmr_1_1_Model.cpp
    src/core/models/titrations/nmr/nmr_2_1_1_1_1_2_Model.cpp
    src/core/models/titrations/nmr/nmr_2_1_1_1_Model.cpp
    src/core/models/titrations/nmr/nmr_any_Model.cpp

    src/core/models/titrations/uv_vis/uv_vis_1_1_1_2_Model.cpp
    src/core/models/titrations/uv_vis/uv_vis_1_1_Model.cpp
    src/core/models/titrations/uv_vis/uv_vis_2_1_1_1_1_2_Model.cpp
    src/core/models/titrations/uv_vis/uv_vis_2_1_1_1_Model.cpp
    src/core/models/titrations/uv_vis/uvvis_any_Model.cpp

 )

IF(Use_Duktape)
    set(duke_SRC
        external/duktape/src/duktape.c
        src/core/models/dukmodelinterpreter.cpp
        )
    set_source_files_properties(external/duktape/src/duktape.c PROPERTIES LANGUAGE CXX)
    add_library(duke SHARED ${duke_SRC})
ENDIF()

if(_Python)
    add_library(models SHARED ${models_SRC})
else()
    add_library(models ${models_SRC})
endif()

target_link_libraries(models Qt::Core ${PYTHON_LIBRARIES} Qt::Qml)

IF(Use_Duktape)
    target_link_libraries(models duke)
ENDIF()

add_subdirectory(src/ui)

set(suprafit_SRC
    src/ui/instance.h
    src/ui/main.cpp
)
qt_add_resources(suprafit_SRC files.qrc)

if(noto_font)
    qt_add_resources(suprafit_SRC fonts.qrc)
endif(noto_font)


# generate proper GUI program on specified platform
if(WIN32) # Check if we are on Windows
    add_executable(suprafit ${suprafit_SRC} suprafit.rc)

        if(MSVC) # Check if we are using the Visual Studio compiler
            set_target_properties(suprafit PROPERTIES
                        WIN32_EXECUTABLE YES
                        LINK_FLAGS "/ENTRY:mainCRTStartup"
                )
            #set_property(TARGET suprafit PROPERTY
            #MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

        elseif(CMAKE_COMPILER_IS_GNUCXX)
                         SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Not tested but seems to work
        else()
                message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
        endif(MSVC)
        set(target_dir "bin/win")
        set_target_properties(suprafit PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )

elseif(APPLE)

    set(SupraFit_icon ${CMAKE_CURRENT_SOURCE_DIR}/misc/SupraFit.icns)
    set_source_files_properties(${SupraFit_icon} PROPERTIES
          MACOSX_PACKAGE_LOCATION "Resources")
    set(MACOSX_BUNDLE_ICON_FILE "SupraFit.icns")

    set(target_dir "bin/macOS")
    add_executable(suprafit MACOSX_BUNDLE ${SupraFit_icon} ${suprafit_SRC})

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    set(target_dir "bin/linux")

    add_executable(suprafit ${suprafit_SRC})
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

IF(_Python)
    set(python_cli_SRCS
                    src/python/main.cpp
              )

    add_executable(python_cli ${python_cli_SRCS})

    set_target_properties(python_cli PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )

    add_library(pythonbridge SHARED ${python_SRC})
    set_property(TARGET pythonbridge PROPERTY CXX_STANDARD 17)
    set_target_properties(pythonbridge PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )
    target_link_libraries(pythonbridge models core)
    target_link_libraries(python_cli pythonbridge)
    set_property(TARGET python_cli PROPERTY CXX_STANDARD 17)

ENDIF()

set_property(TARGET core models PROPERTY CXX_STANDARD 17)
set_property(TARGET suprafit PROPERTY CXX_STANDARD 17)
set_property(TARGET suprafit_cli PROPERTY CXX_STANDARD 17)


set_target_properties(core PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )
set_target_properties(models PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${target_dir} )

target_link_libraries(suprafit suprafit_gui models core cutechart fmt::fmt-header-only)

target_link_libraries(suprafit_cli models core fmt::fmt-header-only)

if(testing)
    enable_testing()
    set(basic_test
        test/save_and_load.cpp
    )
    add_test( basic_test supratest )
    add_executable(supratest  ${basic_test})
    #set_property(TARGET suprafit PROPERTY ENABLE_EXPORTS true)
    set_property(TARGET supratest PROPERTY CXX_STANDARD 17)
    target_link_libraries(supratest Qt::Core Qt::Test)
    target_link_libraries(supratest utils models math suprafit ${QT_QTCORE_LIBRARY})
endif(testing)


if(UNIX)
    target_link_libraries(suprafit pthread dl)
    target_link_libraries(suprafit_cli pthread dl)
endif(UNIX)

# Do platform specific post target stuff
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    install(TARGETS suprafit RUNTIME DESTINATION bin)
    install(TARGETS suprafit_cli RUNTIME DESTINATION bin)
    install(TARGETS models LIBRARY DESTINATION lib)
    install(TARGETS core LIBRARY DESTINATION lib)
    if(_Python)
        install(TARGETS pythonbridge LIBRARY DESTINATION lib)
    ENDIF(_Python)
    #install(FILES src/python/main.py DESTINATION lib)

    # Install desktop entry
    install(FILES misc/SupraFit.desktop DESTINATION share/applications )

    # Install icon
    install(FILES misc/SupraFit.png DESTINATION share/icons/pixmaps/)
elseif(APPLE)
    #string(REPLACE "qmake" "" qt_dir ${QT_QMAKE_EXECUTABLE})
 #   add_custom_command(TARGET suprafit POST_BUILD
 #       COMMAND ${QT_INSTALL_BINS}/macdeployqt  suprafit.app -dmg -no-plugins
 #       WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${target_dir}
 #       )
elseif(WIN32)
    #string(REPLACE "qmake" "" qt_dir ${QT_QMAKE_EXECUTABLE})
    #add_custom_command(TARGET suprafit POST_BUILD
    #    COMMAND ${qt_dir}/windeployqt --release   suprafit.app
    #    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${target_dir}
    #    )
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
