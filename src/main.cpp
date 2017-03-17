#include "ui/mainwindow/suprafit.h"
#include "src/version.h"

#include <QtWidgets/QApplication>

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
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
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
    app.setOrganizationName("Conrad Huebler");
//     app.setApplicationVersion(version);
//     app.setProperty("GIT_BRANCH", git_branch);
//     app.setProperty("GIT_COMMIT_HASH", git_commit_hash);
    MainWindow mainwindow;
    mainwindow.show();
    return app.exec();
}
