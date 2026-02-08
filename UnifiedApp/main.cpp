#include "mainwindow.h"
#include "logindialog.h"
#include <QApplication>
#include <QFont>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Radar System Enterprise");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("Radar Systems Inc.");
    
    // Set modern application-wide font
    QFont appFont("Segoe UI", 10);
    appFont.setStyleHint(QFont::SansSerif);
    app.setFont(appFont);
    
    // Enable high DPI scaling
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // Set modern style hints
    app.setStyle("Fusion");
    
    // Show login dialog with enterprise styling
    LoginDialog loginDialog;
    if (loginDialog.exec() == QDialog::Accepted) {
        // User authenticated successfully
        UserRole userRole = loginDialog.getUserRole();
        QString username = loginDialog.getUsername();
        
        // Create and show main window with appropriate role
        MainWindow window(userRole, username);
        window.show();
        
        return app.exec();
    }
    
    // User cancelled login
    return 0;
}
