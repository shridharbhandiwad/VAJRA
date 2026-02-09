#include "mainwindow.h"
#include "logindialog.h"
#include "componentregistry.h"
#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Radar System Enterprise");
    app.setApplicationVersion("3.0");
    app.setOrganizationName("Radar Systems Inc.");
    
    // Set modern application-wide font
    QFont appFont("Segoe UI", 10);
    appFont.setStyleHint(QFont::SansSerif);
    app.setFont(appFont);
    
    // Enable high DPI scaling
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // Set modern Fusion style
    app.setStyle("Fusion");
    
    // Initialize the component registry from JSON config
    // This is the core of the modular architecture - components are defined
    // in components.json, not in code. New components can be added without
    // any code changes.
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (!registry.loadFromFile()) {
        qWarning() << "[Main] Could not load components.json - starting with empty registry.";
        qWarning() << "[Main] Use the 'Add Component Type' button to define components.";
    } else {
        qDebug() << "[Main] Component registry loaded:"
                 << registry.componentCount() << "types,"
                 << registry.getCategories().size() << "categories";
    }
    
    // Show login dialog
    LoginDialog loginDialog;
    if (loginDialog.exec() == QDialog::Accepted) {
        UserRole userRole = loginDialog.getUserRole();
        QString username = loginDialog.getUsername();
        
        MainWindow window(userRole, username);
        window.show();
        
        return app.exec();
    }
    
    return 0;
}
