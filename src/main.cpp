#include "ui/nmr2fit.h"

#include <QtWidgets/QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    
    app.setApplicationName("nmr2fit placeholder");
    app.setOrganizationName("Conrad Huebler");
    

    MainWindow mainwindow;
    mainwindow.show();
    return app.exec();
}
