#include <QGuiApplication>
#include <QQmlApplicationEngine>

#ifdef BUILD_QT_WIDGETS_UI
#include <QApplication>
#include "MainWindow.h"
#include "wake/WakeWordEngine.h"
#include "wake/ModelManager.h"
#endif

int main(int argc, char* argv[]) {
#ifdef BUILD_QT_WIDGETS_UI
    // When built with Widgets support, show the drag-and-drop model window
    // alongside the QML UI.
    QApplication app(argc, argv);

    // Create the wake word engine and model manager.
    atlas::wake::WakeWordEngine engine;
    atlas::wake::ModelManager manager(engine, "./models");
    manager.scanAndLoad();

    // Show the drag-and-drop window.
    atlas::ui::MainWindow mainWindow(engine, manager);
    mainWindow.show();
#else
    QGuiApplication app(argc, argv);
#endif

    QQmlApplicationEngine qmlEngine;
    qmlEngine.loadFromModule("AtlasUI", "Main");
    if (qmlEngine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
