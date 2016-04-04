#include "ui/nmr2fit.h"

#include <QtWidgets/QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    MainWindow mainwindow;
    mainwindow.show();
    return app.exec();
}
