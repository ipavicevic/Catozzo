#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "ProjectModel.h"
#include "ClipScanner.h"
#include "FfmpegRunner.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Catozzo");
    app.setApplicationName("Catozzo");
    app.setApplicationVersion(APP_VERSION);

    qmlRegisterType<ProjectModel>("Catozzo", 1, 0, "ProjectModel");
    qmlRegisterType<ClipScanner>("Catozzo", 1, 0, "ClipScanner");
    qmlRegisterType<FfmpegRunner>("Catozzo", 1, 0, "FfmpegRunner");

    QQmlApplicationEngine engine;
    engine.loadFromModule("Catozzo", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
