#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QToolBar>
#include <QDebug>

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
    m_userLabel = new QLabel(QString("Logged in as: %1 (%2)")
        .arg(m_username)
        .arg(m_userRole == UserRole::Designer ? "Designer" : "User"), this);
    m_userLabel->setStyleSheet("color: #2c3e50; font-weight: bold; padding: 5px 10px;");
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
    
    // Add designer-specific buttons
    QPushButton* saveBtn = new QPushButton("Save Design", this);
    QPushButton* loadBtn = new QPushButton("Load Design", this);
    QPushButton* clearBtn = new QPushButton("Clear Canvas", this);
    
    saveBtn->setStyleSheet("QPushButton { padding: 5px 15px; }");
    loadBtn->setStyleSheet("QPushButton { padding: 5px 15px; }");
    clearBtn->setStyleSheet("QPushButton { padding: 5px 15px; }");
    
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(loadBtn);
    toolbar->addWidget(clearBtn);
    
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveDesign);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearCanvas);
    
    // Create main widget and layout
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    // Left panel - Components List
    QWidget* leftPanel = new QWidget(this);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    
    QLabel* componentsLabel = new QLabel("Radar Subsystems", leftPanel);
    QFont font = componentsLabel->font();
    font.setPointSize(12);
    font.setBold(true);
    componentsLabel->setFont(font);
    
    m_componentList = new ComponentList(leftPanel);
    
    leftLayout->addWidget(componentsLabel);
    leftLayout->addWidget(m_componentList);
    leftPanel->setLayout(leftLayout);
    leftPanel->setMaximumWidth(200);
    
    // Center panel - Canvas
    QWidget* centerPanel = new QWidget(this);
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    
    QLabel* canvasLabel = new QLabel("Designer View", centerPanel);
    canvasLabel->setFont(font);
    
    m_canvas = new Canvas(centerPanel);
    
    QLabel* hintLabel = new QLabel("Drag and drop components onto canvas", centerPanel);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet("color: gray; font-style: italic;");
    
    centerLayout->addWidget(canvasLabel);
    centerLayout->addWidget(hintLabel);
    centerLayout->addWidget(m_canvas);
    centerPanel->setLayout(centerLayout);
    
    // Right panel - Analytics
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    m_analytics = new Analytics(rightPanel);
    
    rightLayout->addWidget(m_analytics);
    rightPanel->setLayout(rightLayout);
    rightPanel->setMaximumWidth(250);
    
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
    
    // Add runtime-specific buttons
    QPushButton* loadBtn = new QPushButton("Load Design", this);
    loadBtn->setStyleSheet("QPushButton { padding: 5px 15px; }");
    
    m_statusLabel = new QLabel("Server Status: Initializing...", this);
    m_statusLabel->setStyleSheet("padding: 5px 10px;");
    
    toolbar->addWidget(loadBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_statusLabel);
    
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    
    // Start message server
    m_messageServer = new MessageServer(this);
    if (m_messageServer->startServer(12345)) {
        m_statusLabel->setText("Server Status: Running on port 12345 | Clients: 0");
    } else {
        m_statusLabel->setText("Server Status: Failed to start");
        QMessageBox::warning(this, "Error", "Failed to start message server");
    }
    
    connect(m_messageServer, &MessageServer::messageReceived, 
            this, &MainWindow::onMessageReceived);
    connect(m_messageServer, &MessageServer::clientConnected,
            this, &MainWindow::onClientConnected);
    connect(m_messageServer, &MessageServer::clientDisconnected,
            this, &MainWindow::onClientDisconnected);
    
    // Create main widget and layout
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    
    // Center panel - Canvas
    QWidget* centerPanel = new QWidget(this);
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    
    QLabel* canvasLabel = new QLabel("Radar System View", centerPanel);
    QFont font = canvasLabel->font();
    font.setPointSize(12);
    font.setBold(true);
    canvasLabel->setFont(font);
    
    m_canvas = new Canvas(centerPanel);
    
    QLabel* hintLabel = new QLabel("Load a system layout to monitor subsystems", centerPanel);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setStyleSheet("color: gray; font-style: italic;");
    
    centerLayout->addWidget(canvasLabel);
    centerLayout->addWidget(hintLabel);
    centerLayout->addWidget(m_canvas);
    centerPanel->setLayout(centerLayout);
    
    // Right panel - Analytics
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    
    m_analytics = new Analytics(rightPanel);
    
    rightLayout->addWidget(m_analytics);
    rightPanel->setLayout(rightLayout);
    rightPanel->setMaximumWidth(300);
    
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
        return;
    }
    
    // Try to auto-load radar_system.design from the workspace root
    QString fileName = "radar_system.design";
    QFile file(fileName);
    
    if (!file.exists()) {
        qDebug() << "No radar_system.design found in current directory, skipping auto-load";
        return;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Found radar_system.design but could not open it";
        return;
    }
    
    QString json = file.readAll();
    file.close();
    
    m_analytics->clear();
    m_canvas->loadFromJson(json);
    
    qDebug() << "Auto-loaded radar_system.design successfully";
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Server Status: Running on port 12345 | Clients: %1 | Design: radar_system.design")
            .arg(m_connectedClients));
    }
}

void MainWindow::onComponentAdded(const QString& id, ComponentType type)
{
    m_analytics->addComponent(id, getComponentTypeName(type));
}

void MainWindow::onMessageReceived(const QString& componentId, const QString& color, qreal size)
{
    Component* comp = m_canvas->getComponentById(componentId);
    
    if (comp) {
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
        m_statusLabel->setText(QString("Server Status: Running on port 12345 | Clients: %1")
            .arg(m_connectedClients));
    }
}

void MainWindow::onClientDisconnected()
{
    m_connectedClients--;
    if (m_connectedClients < 0) m_connectedClients = 0;
    if (m_statusLabel) {
        m_statusLabel->setText(QString("Server Status: Running on port 12345 | Clients: %1")
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
