#include "mainwindow.h"
#include "componentregistry.h"
#include "addcomponentdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QToolBar>
#include <QDebug>
#include <QDir>
#include <QFrame>

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
    , m_voiceAlertManager(nullptr)
    , m_voiceToggleBtn(nullptr)
{
    setupUI();
    
    if (m_userRole == UserRole::Designer) {
        setWindowTitle("Radar System - Designer Mode");
    } else {
        setWindowTitle("Radar System - Runtime Monitor");
    }
    
    resize(1280, 780);
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
    m_userLabel = new QLabel(QString("  %1  |  %2  ")
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
    QToolBar* toolbar = this->findChild<QToolBar*>();
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    
    // Designer toolbar buttons
    QPushButton* saveBtn = new QPushButton("SAVE DESIGN", this);
    saveBtn->setObjectName("saveButton");
    saveBtn->setToolTip("Save the current radar system design");
    
    QPushButton* loadBtn = new QPushButton("LOAD DESIGN", this);
    loadBtn->setObjectName("loadButton");
    loadBtn->setToolTip("Load an existing radar system design");
    
    QPushButton* clearBtn = new QPushButton("CLEAR CANVAS", this);
    clearBtn->setObjectName("clearButton");
    clearBtn->setToolTip("Clear all components from the canvas");
    
    QPushButton* addTypeBtn = new QPushButton("+ ADD COMPONENT TYPE", this);
    addTypeBtn->setObjectName("addTypeButton");
    addTypeBtn->setToolTip("Add a new component type to the registry (no code changes needed)");
    
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(loadBtn);
    toolbar->addWidget(clearBtn);
    toolbar->addSeparator();
    toolbar->addWidget(addTypeBtn);
    
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveDesign);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearCanvas);
    connect(addTypeBtn, &QPushButton::clicked, this, &MainWindow::addNewComponentType);
    
    // Create main widget and layout
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    
    // ========== LEFT PANEL - Components List ==========
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(14, 14, 14, 14);
    
    QLabel* componentsLabel = new QLabel("COMPONENTS", leftPanel);
    componentsLabel->setObjectName("componentsLabel");
    
    // Component count badge
    ComponentRegistry& registry = ComponentRegistry::instance();
    QLabel* countLabel = new QLabel(
        QString("%1 types available").arg(registry.componentCount()), leftPanel);
    countLabel->setObjectName("countLabel");
    
    m_componentList = new ComponentList(leftPanel);
    m_componentList->setObjectName("componentList");
    
    // Inline add button in the panel
    QPushButton* addInlineBtn = new QPushButton("+ New Type", leftPanel);
    addInlineBtn->setObjectName("addInlineButton");
    addInlineBtn->setCursor(Qt::PointingHandCursor);
    connect(addInlineBtn, &QPushButton::clicked, this, &MainWindow::addNewComponentType);
    
    leftLayout->addWidget(componentsLabel);
    leftLayout->addWidget(countLabel);
    leftLayout->addWidget(m_componentList);
    leftLayout->addWidget(addInlineBtn);
    leftPanel->setLayout(leftLayout);
    leftPanel->setMaximumWidth(240);
    leftPanel->setMinimumWidth(210);
    
    // Update count when registry changes
    connect(&registry, &ComponentRegistry::registryChanged, this, [countLabel, &registry]() {
        countLabel->setText(QString("%1 types available").arg(registry.componentCount()));
    });
    
    // ========== CENTER PANEL - Canvas ==========
    QWidget* centerPanel = new QWidget(this);
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(8);
    centerLayout->setContentsMargins(14, 14, 14, 14);
    
    QLabel* canvasLabel = new QLabel("DESIGNER VIEW", centerPanel);
    canvasLabel->setProperty("heading", true);
    
    m_canvas = new Canvas(centerPanel);
    m_canvas->setObjectName("mainCanvas");
    
    QLabel* hintLabel = new QLabel(
        "Drag components from the left panel onto the canvas. "
        "Use '+ Add Component Type' to define new types without code changes.", centerPanel);
    hintLabel->setProperty("hint", true);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);
    
    centerLayout->addWidget(canvasLabel);
    centerLayout->addWidget(m_canvas);
    centerLayout->addWidget(hintLabel);
    centerPanel->setLayout(centerLayout);
    
    // ========== RIGHT PANEL - Analytics ==========
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(14, 14, 14, 14);
    
    QLabel* analyticsLabel = new QLabel("ANALYTICS", rightPanel);
    analyticsLabel->setProperty("heading", true);
    
    m_analytics = new Analytics(rightPanel);
    m_analytics->setObjectName("analyticsPanel");
    
    rightLayout->addWidget(analyticsLabel);
    rightLayout->addWidget(m_analytics);
    rightPanel->setLayout(rightLayout);
    rightPanel->setMaximumWidth(300);
    rightPanel->setMinimumWidth(250);
    
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
    QToolBar* toolbar = this->findChild<QToolBar*>();
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    
    // Runtime toolbar buttons
    QPushButton* loadBtn = new QPushButton("LOAD DESIGN", this);
    loadBtn->setObjectName("loadButton");
    loadBtn->setToolTip("Load a radar system design file");
    
    m_statusLabel = new QLabel("STATUS: INITIALIZING", this);
    m_statusLabel->setObjectName("statusLabel");
    
    // Voice alert mute/unmute toggle button
    m_voiceToggleBtn = new QPushButton("VOICE ALERTS: ON", this);
    m_voiceToggleBtn->setObjectName("voiceToggleBtn");
    m_voiceToggleBtn->setToolTip("Toggle voice-based health status alerts");
    m_voiceToggleBtn->setCheckable(true);
    m_voiceToggleBtn->setChecked(true);
    m_voiceToggleBtn->setStyleSheet(
        "QPushButton { background: #1b5e20; color: #a5d6a7; border: 1px solid #2e7d32; "
        "border-radius: 4px; padding: 4px 12px; font-size: 10px; font-weight: bold; }"
        "QPushButton:hover { background: #2e7d32; }"
        "QPushButton:checked { background: #1b5e20; color: #a5d6a7; }"
        "QPushButton:!checked { background: #b71c1c; color: #ef9a9a; border-color: #c62828; }");
    connect(m_voiceToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleVoiceAlerts);
    
    toolbar->addWidget(loadBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_statusLabel);
    toolbar->addSeparator();
    toolbar->addWidget(m_voiceToggleBtn);
    
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    
    // Initialize voice alert manager
    m_voiceAlertManager = new VoiceAlertManager(this);
    
    // Start message server
    m_messageServer = new MessageServer(this);
    if (m_messageServer->startServer(12345)) {
        m_statusLabel->setText("STATUS: ACTIVE  |  PORT: 12345  |  CLIENTS: 0");
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
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    
    // Center panel - Canvas
    QWidget* centerPanel = new QWidget(this);
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(8);
    centerLayout->setContentsMargins(14, 14, 14, 14);
    
    QLabel* canvasLabel = new QLabel("SYSTEM MONITOR", centerPanel);
    canvasLabel->setProperty("heading", true);
    
    m_canvas = new Canvas(centerPanel);
    m_canvas->setObjectName("mainCanvas");
    
    QLabel* hintLabel = new QLabel(
        "Load a system layout to monitor subsystems in real-time", centerPanel);
    hintLabel->setProperty("hint", true);
    hintLabel->setAlignment(Qt::AlignCenter);
    
    centerLayout->addWidget(canvasLabel);
    centerLayout->addWidget(m_canvas);
    centerLayout->addWidget(hintLabel);
    centerPanel->setLayout(centerLayout);
    
    // Right panel - Analytics
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(14, 14, 14, 14);
    
    QLabel* analyticsLabel = new QLabel("HEALTH ANALYTICS", rightPanel);
    analyticsLabel->setProperty("heading", true);
    
    m_analytics = new Analytics(rightPanel);
    m_analytics->setObjectName("analyticsPanel");
    
    rightLayout->addWidget(analyticsLabel);
    rightLayout->addWidget(m_analytics);
    rightPanel->setLayout(rightLayout);
    rightPanel->setMaximumWidth(340);
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
    
    if (fileName.isEmpty()) return;
    
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
    
    if (fileName.isEmpty()) return;
    
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
        foreach (Component* comp, m_canvas->getComponents()) {
            onComponentAdded(comp->getId(), comp->getTypeId());
        }
        QMessageBox::information(this, "Success", "Design loaded successfully!");
    } else {
        QMessageBox::information(this, "Success", 
            "Radar system layout loaded!\nWaiting for health updates...");
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

void MainWindow::addNewComponentType()
{
    AddComponentDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        ComponentDefinition def = dialog.getComponentDefinition();
        
        // Refresh the component list
        if (m_componentList) {
            m_componentList->refreshFromRegistry();
        }
        
        QMessageBox::information(this, "Component Added",
            QString("New component type '%1' has been added!\n\n"
                    "It is now available in the component list for drag-and-drop.\n"
                    "The definition has been saved to components.json.")
                .arg(def.displayName));
    }
}

void MainWindow::autoLoadDesign()
{
    if (m_userRole != UserRole::User) return;
    
    QStringList searchPaths;
    searchPaths << "radar_system.design"
                << "../radar_system.design"
                << "/workspace/radar_system.design";
    
    QString foundPath;
    for (const QString& path : searchPaths) {
        if (QFile::exists(path)) {
            foundPath = path;
            break;
        }
    }
    
    if (foundPath.isEmpty()) return;
    
    QFile file(foundPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    
    QString json = file.readAll();
    file.close();
    
    m_analytics->clear();
    m_canvas->loadFromJson(json);
    
    qDebug() << "[MainWindow] Auto-loaded design from:" << foundPath;
    if (m_statusLabel) {
        m_statusLabel->setText(QString("STATUS: ACTIVE  |  PORT: 12345  |  CLIENTS: %1  |  DESIGN LOADED")
            .arg(m_connectedClients));
    }
}

void MainWindow::onComponentAdded(const QString& id, const QString& typeId)
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    QString displayName = typeId;
    if (registry.hasComponent(typeId)) {
        displayName = registry.getComponent(typeId).displayName;
    }
    m_analytics->addComponent(id, displayName);
}

void MainWindow::onMessageReceived(const QString& componentId, const QString& color, qreal size)
{
    Component* comp = m_canvas->getComponentById(componentId);
    
    if (comp) {
        comp->setColor(QColor(color));
        comp->setSize(size);
    } else {
        qDebug() << "[MainWindow] Component" << componentId << "not found in canvas.";
    }
    
    m_analytics->recordMessage(componentId, color, size);
    
    // Trigger voice alert for critical/degraded health states
    if (m_voiceAlertManager) {
        QString componentName = componentId;
        if (comp) {
            componentName = comp->getDisplayName();
        }
        m_voiceAlertManager->processHealthUpdate(componentId, componentName, color, size);
    }
}

void MainWindow::onComponentLoaded(const QString& id, const QString& typeId)
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    QString displayName = typeId;
    if (registry.hasComponent(typeId)) {
        displayName = registry.getComponent(typeId).displayName;
    }
    m_analytics->addComponent(id, displayName);
}

void MainWindow::onClientConnected()
{
    m_connectedClients++;
    if (m_statusLabel) {
        m_statusLabel->setText(QString("STATUS: ACTIVE  |  PORT: 12345  |  CLIENTS: %1")
            .arg(m_connectedClients));
    }
}

void MainWindow::onClientDisconnected()
{
    m_connectedClients--;
    if (m_connectedClients < 0) m_connectedClients = 0;
    if (m_statusLabel) {
        m_statusLabel->setText(QString("STATUS: ACTIVE  |  PORT: 12345  |  CLIENTS: %1")
            .arg(m_connectedClients));
    }
}

void MainWindow::toggleVoiceAlerts()
{
    if (!m_voiceAlertManager || !m_voiceToggleBtn) return;
    
    bool isOn = m_voiceToggleBtn->isChecked();
    m_voiceAlertManager->setMuted(!isOn);
    m_voiceToggleBtn->setText(isOn ? "VOICE ALERTS: ON" : "VOICE ALERTS: OFF");
}
