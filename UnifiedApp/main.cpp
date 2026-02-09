#include "mainwindow.h"
#include "logindialog.h"
#include "componentregistry.h"
#include "thememanager.h"
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
    
    // Initialize theme manager (loads saved preference) and apply theme
    ThemeManager& theme = ThemeManager::instance();
    theme.applyTheme();
    
    // Initialize the component registry from JSON config
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (!registry.loadFromFile()) {
        qWarning() << "[Main] Could not load components.json - starting with empty registry.";
        qWarning() << "[Main] Use the 'Add Component Type' button to define components.";
    } else {
        qDebug() << "[Main] Component registry loaded:"
                 << registry.componentCount() << "types,"
                 << registry.getCategories().size() << "categories";
    }
    
    // Show login dialog (unified - no role selection)
    LoginDialog loginDialog;
    if (loginDialog.exec() == QDialog::Accepted) {
        QString username = loginDialog.getUsername();
        
        // Create unified MainWindow (no separate Designer/Runtime modes)
        MainWindow window(username);
        window.show();
        
        return app.exec();
    }
    
    return 0;
}
