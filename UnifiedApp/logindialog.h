#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

enum class UserRole {
    Designer,
    User
};

class LoginDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit LoginDialog(QWidget* parent = nullptr);
    UserRole getUserRole() const { return m_userRole; }
    QString getUsername() const { return m_username; }
    
private slots:
    void onLoginClicked();
    
private:
    void setupUI();
    
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_loginButton;
    QPushButton* m_cancelButton;
    QLabel* m_errorLabel;
    
    UserRole m_userRole;
    QString m_username;
};

#endif // LOGINDIALOG_H
