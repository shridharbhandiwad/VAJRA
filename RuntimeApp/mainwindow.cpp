#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QToolBar>
#include <QDebug>

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
    
    QPushButton* loadBtn = new QPushButton("Load Design", this);
    m_statusLabel = new QLabel("Server Status: Initializing...", this);
    
    toolbar->addWidget(loadBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_statusLabel);
    
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    
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
    m_statusLabel->setText(QString("Server Status: Running on port 12345 | Clients: %1 | Design: radar_system.design")
        .arg(m_connectedClients));
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
