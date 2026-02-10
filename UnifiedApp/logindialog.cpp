#include "logindialog.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QFile>
#include <QFrame>
#include <QPainter>
#include <QLinearGradient>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QShowEvent>
#include <QTimer>
#include <QDebug>

LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent)
    , m_usernameEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_loginButton(nullptr)
    , m_cancelButton(nullptr)
    , m_togglePasswordBtn(nullptr)
    , m_themeToggleBtn(nullptr)
    , m_errorLabel(nullptr)
    , m_successLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_welcomeLabel(nullptr)
    , m_rememberMeCheck(nullptr)
    , m_opacityEffect(nullptr)
    , m_fadeAnimation(nullptr)
    , m_slideAnimation(nullptr)
    , m_entranceAnimation(nullptr)
    , m_userRole(UserRole::User)
    , m_passwordVisible(false)
    , m_loginAttempts(0)
{
    setupUI();
    setupAnimations();
    
    setWindowTitle("Radar System - Access Control");
    setModal(true);
    setFixedSize(540, 580);
    setObjectName("LoginDialog");
    
    // Standard window frame for professional look
    setWindowFlags(Qt::Dialog);
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    // Listen for theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &LoginDialog::onThemeChanged);
}

void LoginDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 24, 40, 24);
    mainLayout->setSpacing(0);  // Control all spacing manually
    
    // ========== THEME TOGGLE (top right) ==========
    QHBoxLayout* topBarLayout = new QHBoxLayout();
    topBarLayout->setSpacing(0);
    topBarLayout->addStretch();
    
    m_themeToggleBtn = new QPushButton(this);
    m_themeToggleBtn->setObjectName("themeToggleLogin");
    m_themeToggleBtn->setCursor(Qt::PointingHandCursor);
    m_themeToggleBtn->setToolTip("Switch between Dark and Light themes");
    updateThemeButtonText();
    connect(m_themeToggleBtn, &QPushButton::clicked, this, &LoginDialog::onThemeToggle);
    
    topBarLayout->addWidget(m_themeToggleBtn);
    
    mainLayout->addLayout(topBarLayout);
    mainLayout->addSpacing(16);
    
    // ========== HEADER SECTION ==========
    QVBoxLayout* headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);
    
    // Title - Main heading
    m_titleLabel = new QLabel("RADAR MONITORING SYSTEM", this);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    
    // Subtitle
    m_subtitleLabel = new QLabel("ACCESS CONTROL", this);
    m_subtitleLabel->setObjectName("subtitleLabel");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_subtitleLabel);
    
    mainLayout->addLayout(headerLayout);
    mainLayout->addSpacing(20);
    
    // Welcome message
    m_welcomeLabel = new QLabel("Authentication Required", this);
    m_welcomeLabel->setObjectName("welcomeLabel");
    m_welcomeLabel->setAlignment(Qt::AlignCenter);
    
    mainLayout->addWidget(m_welcomeLabel);
    mainLayout->addSpacing(24);
    
    // ========== INPUT FRAME ==========
    QFrame* inputFrame = new QFrame(this);
    inputFrame->setObjectName("inputFrame");
    
    QVBoxLayout* inputLayout = new QVBoxLayout(inputFrame);
    inputLayout->setSpacing(0);
    inputLayout->setContentsMargins(24, 24, 24, 24);
    
    // Username section
    QLabel* usernameLabel = new QLabel("USERNAME", this);
    usernameLabel->setObjectName("fieldLabel");
    
    inputLayout->addWidget(usernameLabel);
    inputLayout->addSpacing(8);
    
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("Enter your username");
    m_usernameEdit->setObjectName("usernameInput");
    
    inputLayout->addWidget(m_usernameEdit);
    inputLayout->addSpacing(20);
    
    // Password section
    QLabel* passwordLabel = new QLabel("PASSWORD", this);
    passwordLabel->setObjectName("fieldLabel");
    
    inputLayout->addWidget(passwordLabel);
    inputLayout->addSpacing(8);
    
    // Password container with integrated show/hide button
    QFrame* passwordContainer = new QFrame(this);
    passwordContainer->setObjectName("passwordContainer");
    
    QHBoxLayout* passwordInputLayout = new QHBoxLayout(passwordContainer);
    passwordInputLayout->setContentsMargins(0, 0, 0, 0);
    passwordInputLayout->setSpacing(0);
    
    m_passwordEdit = new QLineEdit(passwordContainer);
    m_passwordEdit->setPlaceholderText("Enter your password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setObjectName("passwordInput");
    
    m_togglePasswordBtn = new QPushButton("SHOW", passwordContainer);
    m_togglePasswordBtn->setObjectName("togglePassword");
    m_togglePasswordBtn->setFixedHeight(34);
    m_togglePasswordBtn->setMinimumWidth(60);
    m_togglePasswordBtn->setCursor(Qt::PointingHandCursor);
    m_togglePasswordBtn->setToolTip("Toggle password visibility");
    
    passwordInputLayout->addWidget(m_passwordEdit);
    passwordInputLayout->addWidget(m_togglePasswordBtn);
    
    inputLayout->addWidget(passwordContainer);
    inputLayout->addSpacing(18);
    
    // Remember me checkbox
    m_rememberMeCheck = new QCheckBox("Remember me on this device", this);
    m_rememberMeCheck->setChecked(false);
    
    inputLayout->addWidget(m_rememberMeCheck);
    
    mainLayout->addWidget(inputFrame);
    mainLayout->addSpacing(16);
    
    // ========== STATUS MESSAGES ==========
    m_errorLabel = new QLabel(this);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setVisible(false);
    m_errorLabel->setMinimumHeight(0);
    m_errorLabel->setMaximumHeight(0);
    
    m_successLabel = new QLabel(this);
    m_successLabel->setObjectName("successLabel");
    m_successLabel->setAlignment(Qt::AlignCenter);
    m_successLabel->setWordWrap(true);
    m_successLabel->setVisible(false);
    m_successLabel->setMinimumHeight(0);
    m_successLabel->setMaximumHeight(0);
    
    mainLayout->addWidget(m_errorLabel);
    mainLayout->addWidget(m_successLabel);
    mainLayout->addSpacing(16);
    
    // ========== BUTTONS ==========
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);
    
    m_loginButton = new QPushButton("SIGN IN", this);
    m_loginButton->setObjectName("loginButton");
    m_loginButton->setDefault(true);
    m_loginButton->setCursor(Qt::PointingHandCursor);
    m_loginButton->setMinimumHeight(44);
    
    m_cancelButton = new QPushButton("CANCEL", this);
    m_cancelButton->setObjectName("cancelButton");
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setMinimumHeight(44);
    
    buttonLayout->addWidget(m_loginButton);
    buttonLayout->addWidget(m_cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
    
    // ========== FOOTER ==========
    QLabel* footerLabel = new QLabel("RADAR MONITORING SYSTEM v3.0 | AUTHORIZED ACCESS ONLY", this);
    footerLabel->setObjectName("footerLabel");
    footerLabel->setAlignment(Qt::AlignCenter);
    
    mainLayout->addWidget(footerLabel);
    
    // Connect signals
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(m_usernameEdit, &QLineEdit::textChanged, this, &LoginDialog::onUsernameChanged);
    connect(m_passwordEdit, &QLineEdit::textChanged, this, &LoginDialog::onPasswordChanged);
    connect(m_togglePasswordBtn, &QPushButton::clicked, this, &LoginDialog::togglePasswordVisibility);
    
    // Set initial focus
    m_usernameEdit->setFocus();
}

void LoginDialog::setupAnimations()
{
    // Opacity effect for entrance animation
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);
    
    // Fade animation
    m_fadeAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_fadeAnimation->setDuration(800);
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void LoginDialog::applyEntranceAnimation()
{
    if (m_fadeAnimation) {
        m_fadeAnimation->start();
    }
}

void LoginDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    applyEntranceAnimation();
}

void LoginDialog::paintEvent(QPaintEvent* event)
{
    QDialog::paintEvent(event);
}

void LoginDialog::onLoginClicked()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    
    // Hide messages
    m_errorLabel->setVisible(false);
    m_errorLabel->setMinimumHeight(0);
    m_errorLabel->setMaximumHeight(0);
    m_successLabel->setVisible(false);
    m_successLabel->setMinimumHeight(0);
    m_successLabel->setMaximumHeight(0);
    
    // Input validation
    if (username.isEmpty() || password.isEmpty()) {
        m_errorLabel->setText("ERROR: Username and password required");
        m_errorLabel->setMinimumHeight(50);
        m_errorLabel->setMaximumHeight(100);
        m_errorLabel->setVisible(true);
        animateError();
        return;
    }
    
    // Disable button during validation
    m_loginButton->setEnabled(false);
    m_loginButton->setText("VALIDATING...");
    
    // Simulate authentication delay for UX
    QTimer::singleShot(500, this, [this, username, password]() {
        // Validate credentials - role-based access
        if ((username == "Designer" && password == "designer") ||
            (username == "User" && password == "user")) {
            m_username = username;
            
            // Assign role based on username
            if (username == "Designer") {
                m_userRole = UserRole::Designer;
            } else {
                m_userRole = UserRole::User;
            }
            
            QString roleStr = (m_userRole == UserRole::Designer) ? "DESIGNER" : "USER";
            m_successLabel->setText(QString("AUTHENTICATION SUCCESS - %1 ACCESS GRANTED").arg(roleStr));
            m_successLabel->setMinimumHeight(50);
            m_successLabel->setMaximumHeight(100);
            m_successLabel->setVisible(true);
            animateSuccess();
            
            // Close dialog after success animation
            QTimer::singleShot(800, this, &QDialog::accept);
            
        } else {
            m_loginAttempts++;
            QString errorMsg = QString("AUTHENTICATION FAILED - ATTEMPT %1 OF 3").arg(m_loginAttempts);
            
            if (m_loginAttempts >= 3) {
                errorMsg = "ACCESS DENIED - MAXIMUM ATTEMPTS EXCEEDED";
                m_loginButton->setEnabled(false);
                m_usernameEdit->setEnabled(false);
                m_passwordEdit->setEnabled(false);
            } else {
                m_loginButton->setEnabled(true);
                m_loginButton->setText("SIGN IN");
            }
            
            m_errorLabel->setText(errorMsg);
            m_errorLabel->setMinimumHeight(50);
            m_errorLabel->setMaximumHeight(100);
            m_errorLabel->setVisible(true);
            animateError();
            
            m_passwordEdit->clear();
            m_passwordEdit->setFocus();
        }
    });
}

void LoginDialog::onUsernameChanged(const QString& text)
{
    Q_UNUSED(text);
    validateInputs();
}

void LoginDialog::onPasswordChanged(const QString& text)
{
    Q_UNUSED(text);
    validateInputs();
}

void LoginDialog::validateInputs()
{
    bool hasUsername = !m_usernameEdit->text().trimmed().isEmpty();
    bool hasPassword = !m_passwordEdit->text().isEmpty();
    
    // Enable login button only if both fields have text
    if (m_loginAttempts < 3) {
        m_loginButton->setEnabled(hasUsername && hasPassword);
    }
    
    // Hide error when user starts typing
    if (m_errorLabel->isVisible() && (hasUsername || hasPassword)) {
        m_errorLabel->setVisible(false);
        m_errorLabel->setMinimumHeight(0);
        m_errorLabel->setMaximumHeight(0);
    }
}

void LoginDialog::togglePasswordVisibility()
{
    m_passwordVisible = !m_passwordVisible;
    
    if (m_passwordVisible) {
        m_passwordEdit->setEchoMode(QLineEdit::Normal);
        m_togglePasswordBtn->setText("HIDE");
    } else {
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_togglePasswordBtn->setText("SHOW");
    }
}

void LoginDialog::animateError()
{
    // Shake animation for error
    QPropertyAnimation* shakeAnim = new QPropertyAnimation(m_errorLabel, "pos", this);
    shakeAnim->setDuration(500);
    shakeAnim->setLoopCount(1);
    
    QPoint originalPos = m_errorLabel->pos();
    shakeAnim->setKeyValueAt(0.0, originalPos);
    shakeAnim->setKeyValueAt(0.1, originalPos + QPoint(-5, 0));
    shakeAnim->setKeyValueAt(0.2, originalPos + QPoint(5, 0));
    shakeAnim->setKeyValueAt(0.3, originalPos + QPoint(-5, 0));
    shakeAnim->setKeyValueAt(0.4, originalPos + QPoint(5, 0));
    shakeAnim->setKeyValueAt(0.5, originalPos + QPoint(-5, 0));
    shakeAnim->setKeyValueAt(0.6, originalPos + QPoint(5, 0));
    shakeAnim->setKeyValueAt(0.7, originalPos + QPoint(-3, 0));
    shakeAnim->setKeyValueAt(0.8, originalPos + QPoint(3, 0));
    shakeAnim->setKeyValueAt(0.9, originalPos + QPoint(-2, 0));
    shakeAnim->setKeyValueAt(1.0, originalPos);
    
    shakeAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void LoginDialog::animateSuccess()
{
    // Pulse animation for success
    QGraphicsOpacityEffect* successEffect = new QGraphicsOpacityEffect(m_successLabel);
    m_successLabel->setGraphicsEffect(successEffect);
    
    QPropertyAnimation* pulseAnim = new QPropertyAnimation(successEffect, "opacity", this);
    pulseAnim->setDuration(600);
    pulseAnim->setStartValue(0.3);
    pulseAnim->setEndValue(1.0);
    pulseAnim->setEasingCurve(QEasingCurve::OutCubic);
    pulseAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

// ── Theme handling ─────────────────────────────────────────────

void LoginDialog::onThemeToggle()
{
    ThemeManager::instance().toggleTheme();
}

void LoginDialog::onThemeChanged(AppTheme theme)
{
    Q_UNUSED(theme);
    updateThemeButtonText();
}

void LoginDialog::updateThemeButtonText()
{
    if (!m_themeToggleBtn) return;
    
    ThemeManager& tm = ThemeManager::instance();
    if (tm.isDark()) {
        m_themeToggleBtn->setText("LIGHT MODE");
    } else {
        m_themeToggleBtn->setText("DARK MODE");
    }
}
