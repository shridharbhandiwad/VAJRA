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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_canvas(nullptr)
    , m_analytics(nullptr)
    , m_messageServer(nullptr)
    , m_statusLabel(nullptr)
    , m_connectedClients(0)
{
    setupUI();
    
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
    
    setWindowTitle("Radar System Monitor - Real-time Health Monitoring");
    resize(1000, 700);
    
    // Auto-load radar_system.design if it exists
    autoLoadDesign();
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
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    
    QPushButton* loadBtn = new QPushButton("📁 Load Design", this);
    loadBtn->setObjectName("loadButton");
    loadBtn->setToolTip("Load a radar system design file");
    
    m_statusLabel = new QLabel("Server Status: Initializing...", this);
    m_statusLabel->setObjectName("statusLabel");
    
    toolbar->addWidget(loadBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_statusLabel);
    
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    
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
    
    QLabel* canvasLabel = new QLabel("🎯 Radar System View", centerPanel);
    canvasLabel->setProperty("heading", true);
    QFont font = canvasLabel->font();
    font.setPointSize(14);
    font.setBold(true);
    canvasLabel->setFont(font);
    
    m_canvas = new Canvas(centerPanel);
    m_canvas->setObjectName("mainCanvas");
    
    QLabel* hintLabel = new QLabel("💡 Load a system layout to monitor subsystems in real-time", centerPanel);
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
    
    QLabel* analyticsLabel = new QLabel("📊 Health Analytics", rightPanel);
    analyticsLabel->setProperty("heading", true);
    QFont analyticsFont = analyticsLabel->font();
    analyticsFont.setPointSize(14);
    analyticsFont.setBold(true);
    analyticsLabel->setFont(analyticsFont);
    
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
    
    QMessageBox::information(this, "Success", 
        QString("Radar system layout loaded successfully!\n\nWaiting for health updates from subsystems..."));
}

void MainWindow::autoLoadDesign()
{
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
    m_statusLabel->setText(QString("Server Status: Running on port 12345 | Clients: %1 | Design: radar_system.design")
        .arg(m_connectedClients));
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
    m_statusLabel->setText(QString("Server Status: Running on port 12345 | Clients: %1")
        .arg(m_connectedClients));
}

void MainWindow::onClientDisconnected()
{
    m_connectedClients--;
    if (m_connectedClients < 0) m_connectedClients = 0;
    m_statusLabel->setText(QString("Server Status: Running on port 12345 | Clients: %1")
        .arg(m_connectedClients));
}
