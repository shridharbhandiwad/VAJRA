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
    app.setApplicationName("Radar Vital Monitoring System (RVMS)");
    app.setApplicationVersion("3.0");
    app.setOrganizationName("Radar Systems Inc.");
    
    // Set modern application-wide font (Inter / Segoe UI Variable – latest UI fonts)
    QFont appFont("Inter", 12);
    appFont.setStyleHint(QFont::SansSerif);
    // Fallback chain: Inter → Segoe UI Variable → SF Pro Display → Segoe UI → Roboto
    QStringList fontFamilies;
    fontFamilies << "Inter" << "Segoe UI Variable" << "SF Pro Display" << "Segoe UI" << "Roboto" << "Helvetica Neue";
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    appFont.setFamilies(fontFamilies);
#else
    appFont.setFamily(fontFamilies.join(QLatin1Char(',')));
#endif
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
    
    // Show login dialog with role-based access
    LoginDialog loginDialog;
    if (loginDialog.exec() == QDialog::Accepted) {
        QString username = loginDialog.getUsername();
        UserRole role = loginDialog.getUserRole();
        
        // Create MainWindow with role-based access restrictions
        MainWindow window(username, role);
        window.show();
        
        return app.exec();
    }
    
    return 0;
}
