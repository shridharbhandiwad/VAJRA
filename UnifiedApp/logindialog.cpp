#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , m_usernameEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_loginButton(nullptr)
    , m_cancelButton(nullptr)
    , m_errorLabel(nullptr)
    , m_userRole(UserRole::User)
{
    setupUI();
    setWindowTitle("Radar System - Login");
    setModal(true);
    resize(350, 200);
}

void LoginDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Title label
    QLabel* titleLabel = new QLabel("Radar System Application", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #2c3e50; margin: 10px;");
    
    // Info label
    QLabel* infoLabel = new QLabel("Please enter your credentials", this);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("color: #7f8c8d; margin-bottom: 10px;");
    
    // Form layout
    QFormLayout* formLayout = new QFormLayout();
    
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("Enter username");
    
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("Enter password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    
    formLayout->addRow("Username:", m_usernameEdit);
    formLayout->addRow("Password:", m_passwordEdit);
    
    // Error label
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: red; font-weight: bold;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setVisible(false);
    
    // Buttons layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    m_loginButton = new QPushButton("Login", this);
    m_loginButton->setDefault(true);
    m_loginButton->setStyleSheet("QPushButton { background-color: #3498db; color: white; padding: 8px 20px; border-radius: 4px; font-weight: bold; }"
                                  "QPushButton:hover { background-color: #2980b9; }");
    
    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setStyleSheet("QPushButton { padding: 8px 20px; }");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_cancelButton);
    
    // Credentials hint
    QLabel* hintLabel = new QLabel("Valid credentials:\nDesigner/designer (Designer Mode)\nUser/user (Runtime Mode)", this);
    hintLabel->setStyleSheet("color: #95a5a6; font-size: 10px; font-style: italic;");
    hintLabel->setAlignment(Qt::AlignCenter);
    
    // Add all to main layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_errorLabel);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(hintLabel);
    mainLayout->addStretch();
    
    setLayout(mainLayout);
    
    // Connect signals
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::onLoginClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    
    m_errorLabel->setVisible(false);
    
    // Validate credentials
    if (username == "Designer" && password == "designer") {
        m_userRole = UserRole::Designer;
        m_username = username;
        accept();
    } else if (username == "User" && password == "user") {
        m_userRole = UserRole::User;
        m_username = username;
        accept();
    } else {
        m_errorLabel->setText("Invalid username or password!");
        m_errorLabel->setVisible(true);
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}
