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

option(USE_levmar "Use levmar optimizing package" ON)  
option(USE_eigen  "Use Eigen optimizing package" OFF)  

if(USE_levmar)
    if(USE_eigen)
            message( FATAL_ERROR "Can't use levmar and eigen together.")
    endif(USE_eigen)
endif(USE_levmar)

if(USE_eigen)
    if(USE_levmar)
            message( FATAL_ERROR "Can't use levmar and eigen together.")
    endif(USE_levmar)
endif(USE_eigen)

if(NOT USE_eigen)    
    if(NOT USE_levmar)
        message( FATAL_ERROR "We need either levmar or eigen as optimizer.")
    endif(NOT USE_levmar)
endif(NOT USE_eigen)

configure_file (
  "${PROJECT_SOURCE_DIR}/src/global_config.h.in"
  "${PROJECT_BINARY_DIR}/src/global_config.h"
)



if(USE_levmar)
    set(CMAKE_C_FLAGS "-I/usr/include/levmar")
    IF(CMAKE_COMPILER_IS_GNUCXX)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wno-reorder -pedantic-errors \
                                            -Wextra -Wcast-align -Wcast-qual  -Wchar-subscripts  \
                                            -Wcomment -Wconversion -Wdisabled-optimization -Wfloat-equal \
                                            -Wformat  -Wformat=2 -Wformat-nonliteral -Wformat-security\
                                            -Wformat-y2k  -Wimport  -Winit-self  -Winline -Winvalid-pch\
                                            -Wunsafe-loop-optimizations  -Wlong-long -Wmissing-braces\
                                            -Wmissing-field-initializers -Wmissing-format-attribute  \
                                            -Wmissing-include-dirs -Wmissing-noreturn -Wpacked  -Wparentheses\
                                            -Wpointer-arith -Wredundant-decls -Wreturn-type -Wsequence-point\
                                            -Wshadow -Wsign-compare  -Wstack-protector -Wstrict-aliasing\
                                            -Wstrict-aliasing=2 -Wswitch  -Wswitch-default -Wswitch-enum\
                                            -Wtrigraphs  -Wuninitialized -Wunknown-pragmas  -Wunreachable-code\
                                            -Wunused -Wunused-function  -Wunused-label  -Wunused-parameter\
                                            -Wunused-value  -Wunused-variable  -Wvariadic-macros\
                                            -Wvolatile-register-var  -Wwrite-strings")
    ENDIF(CMAKE_COMPILER_IS_GNUCXX)
endif(USE_levmar)

if(USE_eigen)
    find_package( Eigen3 REQUIRED )
    include_directories(${EIGEN3_INCLUDE_DIR})
    # using eigen results in compiler errors so we use fewer flags
    # remove -Wshadow and -Wfloat-equal from list
    # and adding  -Wno-deprecated-declarations
    IF(CMAKE_COMPILER_IS_GNUCXX)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wno-reorder -pedantic-errors \
                                            -Wextra -Wcast-align -Wcast-qual  -Wchar-subscripts  \
                                            -Wcomment -Wconversion -Wdisabled-optimization \
                                            -Wformat  -Wformat=2 -Wformat-nonliteral -Wformat-security\
                                            -Wformat-y2k  -Wimport  -Winit-self  -Winline -Winvalid-pch\
                                            -Wunsafe-loop-optimizations  -Wlong-long -Wmissing-braces\
                                            -Wmissing-field-initializers -Wmissing-format-attribute  \
                                            -Wmissing-include-dirs -Wmissing-noreturn -Wpacked  -Wparentheses\
                                            -Wpointer-arith -Wredundant-decls -Wreturn-type -Wsequence-point\
                                            -Wsign-compare  -Wstack-protector -Wstrict-aliasing\
                                            -Wstrict-aliasing=2 -Wswitch  -Wswitch-default -Wswitch-enum\
                                            -Wtrigraphs  -Wuninitialized -Wunknown-pragmas  -Wunreachable-code\
                                            -Wunused -Wunused-function  -Wunused-label  -Wunused-parameter\
                                            -Wunused-value  -Wunused-variable  -Wvariadic-macros\
                                            -Wvolatile-register-var  -Wwrite-strings -Wno-deprecated-declarations")
    ENDIF(CMAKE_COMPILER_IS_GNUCXX)
endif(USE_eigen)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})



set(nmr2fit_SRCS
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
                src/core/dataclass.cpp
                src/core/AbstractModel.cpp
                src/core/models/1_1_Model.cpp
                src/core/models/2_1_1_1_Model.cpp
                src/core/models/1_1_1_2_Model.cpp
                src/core/jsonhandler.cpp
                src/core/filehandler.cpp
                src/core/libmath.cpp
                src/core/toolset.cpp
                src/global.cpp
                src/main.cpp)
add_executable(nmr2fit ${nmr2fit_SRCS})

set_property(TARGET nmr2fit PROPERTY CXX_STANDARD 11)

qt5_use_modules(nmr2fit Widgets Charts PrintSupport)
if(USE_levmar)
    target_link_libraries(nmr2fit ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTPRINT_LIBRARY} Qt5::PrintSupport levmar)
endif(USE_levmar)

if(USE_eigen)
    target_link_libraries(nmr2fit ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTPRINT_LIBRARY} Qt5::PrintSupport)
endif(USE_eigen)
install(TARGETS nmr2fit RUNTIME DESTINATION bin)
