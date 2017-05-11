#include "src/global_config.h"
#include "src/version.h"
#include "ui/mainwindow/suprafit.h"

#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>

#include <QtWidgets/QApplication>

#include <iostream>
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        if(context.line != 0)
            fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        else
            fprintf(stderr, "Warning: %s \n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}


int main(int argc, char** argv)
{
    
    qInstallMessageHandler(myMessageOutput);
    
    QApplication app(argc, argv);
    
    app.setApplicationName("SupraFit");
    app.setApplicationDisplayName("SupraFit");
    app.setOrganizationName("Conrad Huebler");
    
    app.setApplicationVersion(version);
    
    QCommandLineParser parser;
    parser.setApplicationDescription ( "A Open Source Qt5 based fitting tool for supramolecular titration experiments." );
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("input file", QCoreApplication::translate("main", "File to open."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();

#ifdef _DEBUG
        qDebug() << "Debug output enabled, good fun!";
#endif
    
    
    MainWindow mainwindow;
    mainwindow.show();
    for(const QString &str : qAsConst(args))
        mainwindow.LoadFile(str);
    return app.exec();
}
