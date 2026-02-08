#include "mainwindow.h"
#include "logindialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Show login dialog
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
