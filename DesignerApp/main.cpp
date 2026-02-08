#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QFont>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set modern application-wide font
    QFont appFont("Segoe UI", 10);
    app.setFont(appFont);
    
    // Set application properties
    app.setApplicationName("Radar System - Designer");
    app.setOrganizationName("Radar System");
    app.setApplicationVersion("2.0");
    
    // Load and apply stylesheet
    QFile styleFile(":/styles.qss");
    if (!styleFile.open(QFile::ReadOnly)) {
        qWarning() << "Failed to load stylesheet from resources, trying file system...";
        styleFile.setFileName("styles.qss");
        if (!styleFile.open(QFile::ReadOnly)) {
            qWarning() << "Failed to load stylesheet from file system";
        }
    }
    
    if (styleFile.isOpen()) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "Stylesheet loaded successfully";
    }
    
    // Enable high DPI scaling
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
