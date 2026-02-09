#include "mainwindow.h"
#include "componentregistry.h"
#include "addcomponentdialog.h"
#include "thememanager.h"
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
    , m_connectBtn(nullptr)
    , m_connectionTypeCombo(nullptr)
    , m_themeToggleBtn(nullptr)
    , m_tabWidget(nullptr)
{
    setupUI();
    
    if (m_userRole == UserRole::Designer) {
        setWindowTitle("Radar System - Designer Mode");
    } else {
        setWindowTitle("Radar System - Runtime Monitor");
    }
    
    resize(1400, 850);
    
    // Connect to theme changes for live updates
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
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
    toolbar->addSeparator();
    
    // Connection mode controls
    m_connectBtn = new QPushButton("CONNECT MODE", this);
    m_connectBtn->setObjectName("connectButton");
    m_connectBtn->setCheckable(true);
    m_connectBtn->setToolTip("Toggle connection drawing mode. Click a source component, then drag to target.");
    
    m_connectionTypeCombo = new QComboBox(this);
    m_connectionTypeCombo->setObjectName("connectionTypeCombo");
    m_connectionTypeCombo->addItem("Uni-directional", static_cast<int>(ConnectionType::Unidirectional));
    m_connectionTypeCombo->addItem("Bi-directional", static_cast<int>(ConnectionType::Bidirectional));
    m_connectionTypeCombo->setToolTip("Select connection direction type");
    
    toolbar->addWidget(m_connectBtn);
    toolbar->addWidget(m_connectionTypeCombo);
    toolbar->addSeparator();
    
    // Theme toggle button
    m_themeToggleBtn = new QPushButton(this);
    m_themeToggleBtn->setObjectName("themeToggleBtn");
    m_themeToggleBtn->setCursor(Qt::PointingHandCursor);
    m_themeToggleBtn->setToolTip("Switch between Dark and Light themes");
    updateThemeButtonText();
    toolbar->addWidget(m_themeToggleBtn);
    
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveDesign);
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearCanvas);
    connect(addTypeBtn, &QPushButton::clicked, this, &MainWindow::addNewComponentType);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnectionMode);
    connect(m_connectionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onConnectionTypeChanged);
    connect(m_themeToggleBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggle);
    
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
    
    // Connection help text
    QLabel* connectionHelpLabel = new QLabel(
        "To connect components:\n"
        "1. Click 'CONNECT MODE'\n"
        "2. Choose direction type\n"
        "3. Click source, drag to target\n"
        "4. Enter optional label\n"
        "Press Escape to cancel", leftPanel);
    connectionHelpLabel->setObjectName("connectionHelpLabel");
    connectionHelpLabel->setProperty("hint", true);
    connectionHelpLabel->setWordWrap(true);
    
    leftLayout->addWidget(componentsLabel);
    leftLayout->addWidget(countLabel);
    leftLayout->addWidget(m_componentList);
    leftLayout->addWidget(addInlineBtn);
    leftLayout->addWidget(connectionHelpLabel);
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
        "Use Connect Mode to draw relations between components. "
        "Press Delete to remove selected connections.", centerPanel);
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
    connect(m_canvas, &Canvas::modeChanged, this, &MainWindow::onModeChanged);
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
    connect(m_voiceToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleVoiceAlerts);
    
    // Test voice button to verify audio output
    QPushButton* testVoiceBtn = new QPushButton("TEST VOICE", this);
    testVoiceBtn->setObjectName("testVoiceBtn");
    testVoiceBtn->setToolTip("Test voice output - plays a brief test message");
    connect(testVoiceBtn, &QPushButton::clicked, this, &MainWindow::testVoice);
    
    toolbar->addWidget(loadBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_statusLabel);
    toolbar->addSeparator();
    toolbar->addWidget(m_voiceToggleBtn);
    toolbar->addWidget(testVoiceBtn);
    toolbar->addSeparator();
    
    // Theme toggle button
    m_themeToggleBtn = new QPushButton(this);
    m_themeToggleBtn->setObjectName("themeToggleBtn");
    m_themeToggleBtn->setCursor(Qt::PointingHandCursor);
    m_themeToggleBtn->setToolTip("Switch between Dark and Light themes");
    updateThemeButtonText();
    toolbar->addWidget(m_themeToggleBtn);
    
    connect(loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    connect(m_themeToggleBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggle);
    
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
    
    // ── Central widget with tab view ───────────────────────
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    QVBoxLayout* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setSpacing(0);
    centralLayout->setContentsMargins(8, 8, 8, 8);
    
    // Tab widget for System Overview + per-component enlarged views
    m_tabWidget = new QTabWidget(centralWidget);
    m_tabWidget->setObjectName("componentTabWidget");
    m_tabWidget->setDocumentMode(false);
    m_tabWidget->setTabPosition(QTabWidget::North);
    
    // ── "System Overview" tab (existing canvas + analytics) ─
    QWidget* overviewTab = new QWidget();
    overviewTab->setObjectName("overviewTab");
    QHBoxLayout* overviewLayout = new QHBoxLayout(overviewTab);
    overviewLayout->setSpacing(12);
    overviewLayout->setContentsMargins(10, 10, 10, 10);
    
    // Center panel - Canvas
    QWidget* centerPanel = new QWidget(overviewTab);
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(8);
    centerLayout->setContentsMargins(14, 14, 14, 14);
    
    QLabel* canvasLabel = new QLabel("SYSTEM MONITOR", centerPanel);
    canvasLabel->setProperty("heading", true);
    
    m_canvas = new Canvas(centerPanel);
    m_canvas->setObjectName("mainCanvas");
    
    QLabel* hintLabel = new QLabel(
        "Load a system layout to monitor subsystems in real-time. "
        "Each component shows embedded sub-systems with individual health status. "
        "Click on component tabs above to see enlarged views and detailed analytics.", centerPanel);
    hintLabel->setProperty("hint", true);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);
    
    centerLayout->addWidget(canvasLabel);
    centerLayout->addWidget(m_canvas);
    centerLayout->addWidget(hintLabel);
    
    // Right panel - Analytics
    QWidget* rightPanel = new QWidget(overviewTab);
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
    rightPanel->setMaximumWidth(340);
    rightPanel->setMinimumWidth(280);
    
    overviewLayout->addWidget(centerPanel, 1);
    overviewLayout->addWidget(rightPanel);
    
    // Add overview as first tab
    m_tabWidget->addTab(overviewTab, "  System Overview  ");
    
    centralLayout->addWidget(m_tabWidget);
    setCentralWidget(centralWidget);
    
    // Connect signals
    connect(m_canvas, &Canvas::componentLoaded, this, &MainWindow::onComponentLoaded);
    
    // Auto-load radar_system.design if it exists
    autoLoadDesign();
}

void MainWindow::createComponentTabs()
{
    clearComponentTabs();
    
    QList<Component*> components = m_canvas->getComponents();
    qDebug() << "[MainWindow] Creating enlarged tabs for" << components.size() << "components";
    
    for (Component* comp : components) {
        QString id = comp->getId();
        QString typeId = comp->getTypeId();
        
        // Collect subcomponent names
        QStringList subNames;
        for (SubComponent* sub : comp->getSubComponents()) {
            subNames.append(sub->getName());
        }
        
        EnlargedComponentView* view = new EnlargedComponentView(id, typeId, subNames, m_tabWidget);
        m_enlargedViews[id] = view;
        
        // Set initial health from the component
        view->updateComponentHealth(comp->getColor(), comp->getSize());
        
        // Tab name from display name
        QString displayName = comp->getDisplayName();
        QString tabName = "  " + displayName + "  ";
        m_tabWidget->addTab(view, tabName);
        
        qDebug() << "[MainWindow] Added enlarged tab for" << id << "(" << displayName << ")";
    }
}

void MainWindow::clearComponentTabs()
{
    // Remove all tabs except the first one (System Overview)
    while (m_tabWidget && m_tabWidget->count() > 1) {
        QWidget* w = m_tabWidget->widget(m_tabWidget->count() - 1);
        m_tabWidget->removeTab(m_tabWidget->count() - 1);
        delete w;
    }
    m_enlargedViews.clear();
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
        // Create enlarged component tabs for runtime mode
        createComponentTabs();
        QMessageBox::information(this, "Success", 
            "Radar system layout loaded!\nClick component tabs to see enlarged views and analytics.\nWaiting for health updates...");
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

void MainWindow::toggleConnectionMode()
{
    if (!m_canvas || !m_connectBtn) return;
    
    if (m_connectBtn->isChecked()) {
        m_canvas->setMode(CanvasMode::Connect);
        // Set connection type from combo
        int typeIdx = m_connectionTypeCombo->currentData().toInt();
        m_canvas->setConnectionType(static_cast<ConnectionType>(typeIdx));
    } else {
        m_canvas->setMode(CanvasMode::Select);
    }
}

void MainWindow::onConnectionTypeChanged(int index)
{
    Q_UNUSED(index);
    if (!m_canvas || !m_connectionTypeCombo) return;
    
    int typeVal = m_connectionTypeCombo->currentData().toInt();
    m_canvas->setConnectionType(static_cast<ConnectionType>(typeVal));
}

void MainWindow::onModeChanged(CanvasMode mode)
{
    if (m_connectBtn) {
        m_connectBtn->setChecked(mode == CanvasMode::Connect);
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
    
    // Create enlarged component tabs for runtime mode
    createComponentTabs();
    
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
    
    // Update enlarged view if it exists for this component
    if (m_enlargedViews.contains(componentId)) {
        m_enlargedViews[componentId]->updateComponentHealth(QColor(color), size);
    }
    
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

void MainWindow::testVoice()
{
    if (!m_voiceAlertManager) return;
    
    if (!m_voiceAlertManager->isTtsAvailable()) {
        QMessageBox::warning(this, "Voice Not Available",
            "No text-to-speech engine is installed.\n\n"
            "Install espeak-ng and alsa-utils:\n"
            "  sudo apt-get install espeak-ng alsa-utils\n\n"
            "Then restart the application.");
        return;
    }
    
    m_voiceAlertManager->testVoice();
}

// ── Theme handling ─────────────────────────────────────────────

void MainWindow::onThemeToggle()
{
    ThemeManager::instance().toggleTheme();
}

void MainWindow::onThemeChanged(AppTheme theme)
{
    Q_UNUSED(theme);
    updateThemeButtonText();
    refreshCanvasBackground();
    
    // Force analytics to redraw with new theme colours
    if (m_analytics) {
        m_analytics->updateDisplay();
    }
}

void MainWindow::updateThemeButtonText()
{
    if (!m_themeToggleBtn) return;
    
    ThemeManager& tm = ThemeManager::instance();
    if (tm.isDark()) {
        m_themeToggleBtn->setText("LIGHT MODE");
    } else {
        m_themeToggleBtn->setText("DARK MODE");
    }
}

void MainWindow::refreshCanvasBackground()
{
    if (!m_canvas) return;
    
    ThemeManager& tm = ThemeManager::instance();
    m_canvas->setBackgroundBrush(QBrush(tm.canvasBackground()));
    m_canvas->viewport()->update();
    
    // Force repaint of all scene items (Components, SubComponents, Connections)
    if (m_canvas->scene()) {
        m_canvas->scene()->update();
    }
}
