#include <QtGui/QGuiApplication>
#include <QtQml>
#include "qtquick2applicationviewer.h"

#include "cpp/filetreemodel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QtQuick2ApplicationViewer viewer;

    qmlRegisterType<FileTreeModel>("tk.corvinus.FileTreeModel", 1, 0, "FileTreeModel");

    viewer.setMainQmlFile(QStringLiteral("qml/ImageViewer/main.qml"));
    viewer.showExpanded();

    return app.exec();
}
