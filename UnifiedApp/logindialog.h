#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QCheckBox>
#include "thememanager.h"

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
    
protected:
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    
private slots:
    void onLoginClicked();
    void onUsernameChanged(const QString& text);
    void onPasswordChanged(const QString& text);
    void togglePasswordVisibility();
    void animateError();
    void animateSuccess();
    void onThemeToggle();
    void onThemeChanged(AppTheme theme);
    
private:
    void setupUI();
    void setupAnimations();
    void applyEntranceAnimation();
    void validateInputs();
    void updateThemeButtonText();
    
    // UI Elements
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_loginButton;
    QPushButton* m_cancelButton;
    QPushButton* m_togglePasswordBtn;
    QPushButton* m_themeToggleBtn;
    QLabel* m_errorLabel;
    QLabel* m_successLabel;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLabel* m_welcomeLabel;
    QCheckBox* m_rememberMeCheck;
    
    // Animation Effects
    QGraphicsOpacityEffect* m_opacityEffect;
    QPropertyAnimation* m_fadeAnimation;
    QPropertyAnimation* m_slideAnimation;
    QParallelAnimationGroup* m_entranceAnimation;
    
    // State
    UserRole m_userRole;
    QString m_username;
    bool m_passwordVisible;
    int m_loginAttempts;
};

#endif // LOGINDIALOG_H
