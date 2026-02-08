#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QToolBar>
#include <QDebug>
#include <QDir>

MainWindow::MainWindow(UserRole userRole, const QString& username, QWidget* parent)
    : QMainWindow(parent)
    , m_userRole(userRole)
    , m_username(username)
    , m_componentList(nullptr)
    , m_canvas(nullptr)
    , m_analytics(nullptr)
    , m_messageServer(nullptr)
    , m_statusLabel(nullptr)
    , m_userLabel(nullptr)
    , m_connectedClients(0)
{
    setupUI();
    
    // Set window title based on role
    if (m_userRole == UserRole::Designer) {
        setWindowTitle("Radar System - Designer Mode");
    } else {
        setWindowTitle("Radar System - Runtime Monitor");
    }
    
    resize(1200, 700);
}

MainWindow::~MainWindow()
{
    if (m_messageServer) {
        m_messageServer->stopServer();
    }
}

void MainWindow::setupUI()
{
    // Create toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");
    
    // User info label
    m_userLabel = new QLabel(QString("USER: %1 | MODE: %2")
        .arg(m_username.toUpper())
        .arg(m_userRole == UserRole::Designer ? "DESIGNER" : "RUNTIME"), this);
    m_userLabel->setObjectName("userLabel");
    toolbar->addWidget(m_userLabel);
    toolbar->addSeparator();
    
    if (m_userRole == UserRole::Designer) {
        setupDesignerMode();
    } else {
        setupRuntimeMode();
    }
}

void MainWindow::setupDesignerMode()
{
    // Get toolbar
    QToolBar* toolbar = this->findChild<QToolBar*>();
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    
    // Add designer-specific buttons
    QPushButton* saveBtn = new QPushButton("SAVE DESIGN", this);
    saveBtn->setObjectName("saveButton");
    saveBtn->setToolTip("Save the current radar system design");
    
    QPushButton* loadBtn = new QPushButton("LOAD DESIGN", this);
    loadBtn->setObjectName("loadButton");
    loadBtn->setToolTip("Load an existing radar system design");
    
    QPushButton* clearBtn = new QPushButton("CLEAR CANVAS", this);
    clearBtn->setObjectName("clearButton");
    clearBtn->setToolTip("Clear all components from the canvas");
    
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(loadBtn);
    toolbar->addWidget(clearBtn);
    
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveDesign);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearCanvas);
    
    // Create main widget and layout
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Left panel - Components List
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* componentsLabel = new QLabel("RADAR SUBSYSTEMS", leftPanel);
    componentsLabel->setObjectName("componentsLabel");
    
    m_componentList = new ComponentList(leftPanel);
    m_componentList->setObjectName("componentList");
    
    leftLayout->addWidget(componentsLabel);
    leftLayout->addWidget(m_componentList);
    leftPanel->setLayout(leftLayout);
    leftPanel->setMaximumWidth(230);
    leftPanel->setMinimumWidth(200);
    
    // Center panel - Canvas
    QWidget* centerPanel = new QWidget(this);
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(10);
    centerLayout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* canvasLabel = new QLabel("DESIGNER VIEW", centerPanel);
    canvasLabel->setProperty("heading", true);
    
    m_canvas = new Canvas(centerPanel);
    m_canvas->setObjectName("mainCanvas");
    
    QLabel* hintLabel = new QLabel("Drag and drop radar components onto the canvas to create your system layout", centerPanel);
    hintLabel->setProperty("hint", true);
    hintLabel->setAlignment(Qt::AlignCenter);
    
    centerLayout->addWidget(canvasLabel);
    centerLayout->addWidget(hintLabel);
    centerLayout->addWidget(m_canvas);
    centerPanel->setLayout(centerLayout);
    
    // Right panel - Analytics
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* analyticsLabel = new QLabel("ANALYTICS", rightPanel);
    analyticsLabel->setProperty("heading", true);
    
    m_analytics = new Analytics(rightPanel);
    m_analytics->setObjectName("analyticsPanel");
    
    rightLayout->addWidget(analyticsLabel);
    rightLayout->addWidget(m_analytics);
    rightPanel->setLayout(rightLayout);
    rightPanel->setMaximumWidth(280);
    rightPanel->setMinimumWidth(240);
    
    // Add panels to main layout
    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(centerPanel, 1);
    mainLayout->addWidget(rightPanel);
    
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    
    // Connect signals
    connect(m_canvas, &Canvas::componentAdded, this, &MainWindow::onComponentAdded);
}

void MainWindow::setupRuntimeMode()
{
    // Get toolbar
    QToolBar* toolbar = this->findChild<QToolBar*>();
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    
    // Add runtime-specific buttons
    QPushButton* loadBtn = new QPushButton("LOAD DESIGN", this);
    loadBtn->setObjectName("loadButton");
    loadBtn->setToolTip("Load a radar system design file");
    
    m_statusLabel = new QLabel("STATUS: INITIALIZING", this);
    m_statusLabel->setObjectName("statusLabel");
    
    toolbar->addWidget(loadBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_statusLabel);
    
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    
    // Start message server
    m_messageServer = new MessageServer(this);
    if (m_messageServer->startServer(12345)) {
        m_statusLabel->setText("STATUS: ACTIVE | PORT: 12345 | CLIENTS: 0");
    } else {
        m_statusLabel->setText("STATUS: ERROR - SERVER FAILED");
        QMessageBox::warning(this, "Server Error", "Failed to start message server on port 12345");
    }
    
    connect(m_messageServer, &MessageServer::messageReceived, 
            this, &MainWindow::onMessageReceived);
    connect(m_messageServer, &MessageServer::clientConnected,
            this, &MainWindow::onClientConnected);
    connect(m_messageServer, &MessageServer::clientDisconnected,
            this, &MainWindow::onClientDisconnected);
    
    // Create main widget and layout
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // Center panel - Canvas
    QWidget* centerPanel = new QWidget(this);
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(10);
    centerLayout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* canvasLabel = new QLabel("RADAR SYSTEM VIEW", centerPanel);
    canvasLabel->setProperty("heading", true);
    
    m_canvas = new Canvas(centerPanel);
    m_canvas->setObjectName("mainCanvas");
    
    QLabel* hintLabel = new QLabel("Load a system layout to monitor subsystems in real-time", centerPanel);
    hintLabel->setProperty("hint", true);
    hintLabel->setAlignment(Qt::AlignCenter);
    
    centerLayout->addWidget(canvasLabel);
    centerLayout->addWidget(hintLabel);
    centerLayout->addWidget(m_canvas);
    centerPanel->setLayout(centerLayout);
    
    // Right panel - Analytics
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    
    QLabel* analyticsLabel = new QLabel("HEALTH ANALYTICS", rightPanel);
    analyticsLabel->setProperty("heading", true);
    
    m_analytics = new Analytics(rightPanel);
    m_analytics->setObjectName("analyticsPanel");
    
    rightLayout->addWidget(analyticsLabel);
    rightLayout->addWidget(m_analytics);
    rightPanel->setLayout(rightLayout);
    rightPanel->setMaximumWidth(320);
    rightPanel->setMinimumWidth(280);
    
    // Add panels to main layout
    mainLayout->addWidget(centerPanel, 1);
    mainLayout->addWidget(rightPanel);
    
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    
    // Connect signals
    connect(m_canvas, &Canvas::componentLoaded, this, &MainWindow::onComponentLoaded);
    
    // Auto-load radar_system.design if it exists
    autoLoadDesign();
}

void MainWindow::saveDesign()
{
    if (m_userRole != UserRole::Designer) {
        QMessageBox::warning(this, "Access Denied", "Only Designer users can save designs.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, 
        "Save Design", "", "Design Files (*.design)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QString json = m_canvas->saveToJson();
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not save file");
        return;
    }
    
    file.write(json.toUtf8());
    file.close();
    
    QMessageBox::information(this, "Success", "Design saved successfully!");
}

void MainWindow::loadDesign()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Load Design", "", "Design Files (*.design)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open file");
        return;
    }
    
    QString json = file.readAll();
    file.close();
    
    m_analytics->clear();
    m_canvas->loadFromJson(json);
    
    if (m_userRole == UserRole::Designer) {
        // Update analytics with loaded components
        foreach (Component* comp, m_canvas->getComponents()) {
            onComponentAdded(comp->getId(), comp->getType());
        }
        QMessageBox::information(this, "Success", "Design loaded successfully!");
    } else {
        QMessageBox::information(this, "Success", 
            QString("Radar system layout loaded successfully!\n\nWaiting for health updates from subsystems..."));
    }
}

void MainWindow::clearCanvas()
{
    if (m_userRole != UserRole::Designer) {
        QMessageBox::warning(this, "Access Denied", "Only Designer users can clear the canvas.");
        return;
    }
    
    m_canvas->clearCanvas();
    m_analytics->clear();
}

void MainWindow::autoLoadDesign()
{
    // Only auto-load in runtime mode
    if (m_userRole != UserRole::User) {
        qDebug() << "[MainWindow] autoLoadDesign() skipped - not in User/Runtime mode";
        return;
    }
    
    qDebug() << "[MainWindow] autoLoadDesign() called";
    qDebug() << "[MainWindow] Current working directory:" << QDir::currentPath();
    
    // Try multiple potential locations for the design file
    QStringList searchPaths;
    searchPaths << "radar_system.design";  // Current directory
    searchPaths << "../radar_system.design";  // Parent directory
    searchPaths << "/workspace/radar_system.design";  // Absolute path
    
    QString foundPath;
    for (const QString& path : searchPaths) {
        QFile testFile(path);
        if (testFile.exists()) {
            foundPath = path;
            qDebug() << "[MainWindow] Found design file at:" << path;
            break;
        }
    }
    
    if (foundPath.isEmpty()) {
        qDebug() << "[MainWindow] No radar_system.design found in any search path, skipping auto-load";
        qDebug() << "[MainWindow] Searched paths:" << searchPaths;
        return;
    }
    
    QFile file(foundPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[MainWindow] Found radar_system.design but could not open it:" << file.errorString();
        return;
    }
    
    QString json = file.readAll();
    file.close();
    
    qDebug() << "[MainWindow] Read" << json.length() << "bytes from design file";
    qDebug() << "[MainWindow] Clearing analytics...";
    m_analytics->clear();
    
    qDebug() << "[MainWindow] Loading JSON into canvas...";
    m_canvas->loadFromJson(json);
    
    qDebug() << "[MainWindow] Auto-loaded radar_system.design successfully from:" << foundPath;
    if (m_statusLabel) {
        m_statusLabel->setText(QString("STATUS: ACTIVE | PORT: 12345 | CLIENTS: %1 | DESIGN LOADED")
            .arg(m_connectedClients));
    }
}

void MainWindow::onComponentAdded(const QString& id, ComponentType type)
{
    m_analytics->addComponent(id, getComponentTypeName(type));
}

void MainWindow::onMessageReceived(const QString& componentId, const QString& color, qreal size)
{
    qDebug() << "[MainWindow] onMessageReceived called for component:" << componentId << "color:" << color << "size:" << size;
    Component* comp = m_canvas->getComponentById(componentId);
    
    if (comp) {
        qDebug() << "[MainWindow] Component found! Updating appearance...";
        // Update component appearance
        comp->setColor(QColor(color));
        comp->setSize(size);
    } else {
        qDebug() << "Warning: Component" << componentId << "not found in canvas. Message received but visual not updated.";
    }
    
    // Always update analytics, even if component visual doesn't exist
    m_analytics->recordMessage(componentId, color, size);
}

void MainWindow::onComponentLoaded(const QString& id, const QString& type)
{
    m_analytics->addComponent(id, type);
}

void MainWindow::onClientConnected()
{
    m_connectedClients++;
    if (m_statusLabel) {
        m_statusLabel->setText(QString("STATUS: ACTIVE | PORT: 12345 | CLIENTS: %1")
            .arg(m_connectedClients));
    }
}

void MainWindow::onClientDisconnected()
{
    m_connectedClients--;
    if (m_connectedClients < 0) m_connectedClients = 0;
    if (m_statusLabel) {
        m_statusLabel->setText(QString("STATUS: ACTIVE | PORT: 12345 | CLIENTS: %1")
            .arg(m_connectedClients));
    }
}

QString MainWindow::getComponentTypeName(ComponentType type)
{
    switch (type) {
        case ComponentType::Antenna: return "Antenna";
        case ComponentType::PowerSystem: return "Power System";
        case ComponentType::LiquidCoolingUnit: return "Liquid Cooling Unit";
        case ComponentType::CommunicationSystem: return "Communication System";
        case ComponentType::RadarComputer: return "Radar Computer";
        default: return "Unknown";
    }
}
