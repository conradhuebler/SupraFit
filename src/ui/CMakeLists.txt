cmake_minimum_required(VERSION 3.0.2)
project(SupraFitGui)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)


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
                                           -Wno-error=unused-local-typedefs -Wno-error=enum-compare -Wno-narrowing")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Charts CONFIG REQUIRED)

set(SFGui_SRC
                guitools/chartwrapper.cpp
                guitools/mime.h
                dialogs/spectraimport.cpp
                dialogs/regressionanalysisdialog.cpp
                dialogs/statisticdialog.cpp
                dialogs/modaldialog.cpp
                dialogs/advancedsearch.cpp
                dialogs/importdata.cpp
                dialogs/configdialog.cpp
                dialogs/comparedialog.cpp
                dialogs/resultsdialog.cpp
                dialogs/thermogram.cpp
                widgets/DropTable.h
                widgets/spectrawidget.cpp
                widgets/metamodelparameter.cpp
                widgets/parameterwidget.cpp
                widgets/optimizerwidget.cpp
                widgets/results/scatterwidget.cpp
                widgets/results/searchresultwidget.cpp
                widgets/results/mcresultswidget.cpp
                widgets/results/resultswidget.cpp
                widgets/optionswidget.cpp
                widgets/systemparameterwidget.cpp
                widgets/modelactions.cpp
                widgets/modelchart.cpp
                widgets/modelelement.cpp
                widgets/signalelement.cpp
                widgets/statisticwidget.cpp
                widgets/messagedock.cpp
                widgets/exportsimulationwidget.cpp
                widgets/buttons/spinbox.h
                widgets/buttons/scientificbox.h
                widgets/buttons/hovercheckbox.h
                widgets/thermogramwidget.cpp
                widgets/textwidget.cpp
                mainwindow/chartwidget.cpp
                mainwindow/modeldataholder.cpp
                mainwindow/datawidget.cpp
                mainwindow/metamodelwidget.cpp
                mainwindow/modelwidget.cpp
                mainwindow/mainwindow.cpp
                mainwindow/suprafitgui.cpp
    )

add_library(suprafit_gui ${SFGui_SRC})
set_property(TARGET suprafit_gui PROPERTY CXX_STANDARD 14)

target_link_libraries(suprafit_gui Qt5::Core Qt5::Widgets Qt5::Charts)
