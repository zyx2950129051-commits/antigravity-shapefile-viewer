#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("ShapefileViewer"));
    QApplication::setApplicationDisplayName(QStringLiteral("Shapefile 查看器"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("GeoViewer"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("现代轻量级 C++ Shapefile 桌面查看器"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("启动时自动加载的 Shapefile (*.shp) 路径"), QStringLiteral("[file]"));

    parser.process(app);

    UI::MainWindow mainWindow;
    mainWindow.show();

    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty()) {
        QString initialFile = positionalArgs.first();
        if (QFileInfo::exists(initialFile)) {
            mainWindow.openShapefile(initialFile);
        }
    }

    return app.exec();
}
