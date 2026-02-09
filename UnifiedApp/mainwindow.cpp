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
#include <QSplitter>

MainWindow::MainWindow(const QString& username, UserRole role, QWidget* parent)
    : QMainWindow(parent)
    , m_username(username)
    , m_role(role)
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
    , m_saveBtn(nullptr)
    , m_loadBtn(nullptr)
    , m_clearBtn(nullptr)
    , m_addTypeBtn(nullptr)
    , m_leftPanel(nullptr)
    , m_tabWidget(nullptr)
{
    setupUI();
    
    // Set window title based on role
    QString roleStr = (m_role == UserRole::Designer) ? "Designer" : "Monitor";
    setWindowTitle(QString("Radar System - %1").arg(roleStr));
    resize(1400, 850);
    
    // Connect to theme changes for live updates
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
    
    // Apply role-based restrictions
    applyRoleRestrictions();
    
    // Auto-load design if available
    autoLoadDesign();
}

MainWindow::~MainWindow()
{
    if (m_messageServer) {
        m_messageServer->stopServer();
    }
}

// ======================================================================
// Role-Based Access Restrictions
// ======================================================================

void MainWindow::applyRoleRestrictions()
{
    if (m_role == UserRole::Designer) {
        // Designer: Full design on System Overview canvas only.
        // Hide per-component enlarged view tabs (keep only System Overview).
        // Component tabs are NOT created for the Designer role.
        // (createComponentTabs / addComponentTab are gated below.)
        
    } else if (m_role == UserRole::User) {
        // User: Monitor-only. No design tools, no components panel.
        
        // Hide the left panel (components list)
        if (m_leftPanel) {
            m_leftPanel->setVisible(false);
        }
        
        // Hide design toolbar buttons
        if (m_saveBtn)   m_saveBtn->setVisible(false);
        if (m_loadBtn)   m_loadBtn->setVisible(false);
        if (m_clearBtn)  m_clearBtn->setVisible(false);
        if (m_addTypeBtn) m_addTypeBtn->setVisible(false);
        
        // Hide connection mode controls (design feature)
        if (m_connectBtn) m_connectBtn->setVisible(false);
        if (m_connectionTypeCombo) m_connectionTypeCombo->setVisible(false);
        
        // Set canvas to read-only (no drag-drop, no move, no delete)
        if (m_canvas) {
            m_canvas->setReadOnly(true);
        }
    }
}

void MainWindow::setupUI()
{
    // ========== TOOLBAR ==========
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    
    // User info label with role
    QString roleLabel = (m_role == UserRole::Designer) ? "DESIGNER" : "USER";
    m_userLabel = new QLabel(QString("  %1  |  %2  ").arg(m_username.toUpper(), roleLabel), this);
    m_userLabel->setObjectName("userLabel");
    toolbar->addWidget(m_userLabel);
    toolbar->addSeparator();
    
    // Design toolbar buttons
    m_saveBtn = new QPushButton("SAVE DESIGN", this);
    m_saveBtn->setObjectName("saveButton");
    m_saveBtn->setToolTip("Save the current radar system design");
    
    m_loadBtn = new QPushButton("LOAD DESIGN", this);
    m_loadBtn->setObjectName("loadButton");
    m_loadBtn->setToolTip("Load an existing radar system design");
    
    m_clearBtn = new QPushButton("CLEAR CANVAS", this);
    m_clearBtn->setObjectName("clearButton");
    m_clearBtn->setToolTip("Clear all components from the canvas");
    
    m_addTypeBtn = new QPushButton("+ ADD TYPE", this);
    m_addTypeBtn->setObjectName("addTypeButton");
    m_addTypeBtn->setToolTip("Add a new component type to the registry");
    
    toolbar->addWidget(m_saveBtn);
    toolbar->addWidget(m_loadBtn);
    toolbar->addWidget(m_clearBtn);
    toolbar->addSeparator();
    toolbar->addWidget(m_addTypeBtn);
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
    
    // Runtime status & voice controls
    m_statusLabel = new QLabel("STATUS: INITIALIZING", this);
    m_statusLabel->setObjectName("statusLabel");
    toolbar->addWidget(m_statusLabel);
    toolbar->addSeparator();
    
    m_voiceToggleBtn = new QPushButton("VOICE: ON", this);
    m_voiceToggleBtn->setObjectName("voiceToggleBtn");
    m_voiceToggleBtn->setToolTip("Toggle voice-based health status alerts");
    m_voiceToggleBtn->setCheckable(true);
    m_voiceToggleBtn->setChecked(true);
    
    QPushButton* testVoiceBtn = new QPushButton("TEST VOICE", this);
    testVoiceBtn->setObjectName("testVoiceBtn");
    testVoiceBtn->setToolTip("Test voice output");
    
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
    
    // Connect toolbar signals
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::saveDesign);
    connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::clearCanvas);
    connect(m_addTypeBtn, &QPushButton::clicked, this, &MainWindow::addNewComponentType);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnectionMode);
    connect(m_connectionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onConnectionTypeChanged);
    connect(m_voiceToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleVoiceAlerts);
    connect(testVoiceBtn, &QPushButton::clicked, this, &MainWindow::testVoice);
    connect(m_themeToggleBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggle);
    
    // ========== MAIN LAYOUT ==========
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    
    // ========== LEFT PANEL - Components & Sub-Components List ==========
    m_leftPanel = new QWidget(this);
    m_leftPanel->setObjectName("leftPanel");
    QVBoxLayout* leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(14, 14, 14, 14);
    
    QLabel* componentsLabel = new QLabel("COMPONENTS", m_leftPanel);
    componentsLabel->setObjectName("componentsLabel");
    
    ComponentRegistry& registry = ComponentRegistry::instance();
    QLabel* countLabel = new QLabel(
        QString("%1 types available").arg(registry.componentCount()), m_leftPanel);
    countLabel->setObjectName("countLabel");
    
    m_componentList = new ComponentList(m_leftPanel);
    m_componentList->setObjectName("componentList");
    
    QPushButton* addInlineBtn = new QPushButton("+ New Type", m_leftPanel);
    addInlineBtn->setObjectName("addInlineButton");
    addInlineBtn->setCursor(Qt::PointingHandCursor);
    connect(addInlineBtn, &QPushButton::clicked, this, &MainWindow::addNewComponentType);
    
    QLabel* helpLabel = new QLabel(
        "Drag components onto the canvas.\n"
        "Drag Label/LineEdit/Button into\n"
        "components as sub-widgets.\n\n"
        "Connect Mode: click source,\n"
        "drag to target.\n"
        "Press Delete to remove selected.", m_leftPanel);
    helpLabel->setObjectName("connectionHelpLabel");
    helpLabel->setProperty("hint", true);
    helpLabel->setWordWrap(true);
    
    leftLayout->addWidget(componentsLabel);
    leftLayout->addWidget(countLabel);
    leftLayout->addWidget(m_componentList);
    leftLayout->addWidget(addInlineBtn);
    leftLayout->addWidget(helpLabel);
    m_leftPanel->setLayout(leftLayout);
    m_leftPanel->setMaximumWidth(240);
    m_leftPanel->setMinimumWidth(210);
    
    connect(&registry, &ComponentRegistry::registryChanged, this, [countLabel, &registry]() {
        countLabel->setText(QString("%1 types available").arg(registry.componentCount()));
    });
    
    // ========== CENTER PANEL - Tab Widget (Canvas + Enlarged Views) ==========
    QWidget* centerPanel = new QWidget(this);
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(8);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    
    m_tabWidget = new QTabWidget(centerPanel);
    m_tabWidget->setObjectName("componentTabWidget");
    m_tabWidget->setDocumentMode(false);
    m_tabWidget->setTabPosition(QTabWidget::North);
    
    // System Overview tab with canvas
    QWidget* overviewTab = new QWidget();
    overviewTab->setObjectName("overviewTab");
    QVBoxLayout* overviewLayout = new QVBoxLayout(overviewTab);
    overviewLayout->setSpacing(8);
    overviewLayout->setContentsMargins(8, 8, 8, 8);
    
    QLabel* canvasLabel = new QLabel("DESIGN & MONITOR", overviewTab);
    canvasLabel->setProperty("heading", true);
    
    m_canvas = new Canvas(overviewTab);
    m_canvas->setObjectName("mainCanvas");
    
    QLabel* hintLabel = new QLabel(
        "Drag components from the left panel onto the canvas. "
        "Drop Label/LineEdit/Button sub-components inside parent components. "
        "Use Connect Mode to draw relations. "
        "Health updates appear in real-time when external systems connect.", overviewTab);
    hintLabel->setProperty("hint", true);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);
    
    overviewLayout->addWidget(canvasLabel);
    overviewLayout->addWidget(m_canvas);
    overviewLayout->addWidget(hintLabel);
    
    m_tabWidget->addTab(overviewTab, "  System Overview  ");
    
    centerLayout->addWidget(m_tabWidget);
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
    rightPanel->setMaximumWidth(320);
    rightPanel->setMinimumWidth(260);
    
    // ========== ASSEMBLE MAIN LAYOUT ==========
    mainLayout->addWidget(m_leftPanel);
    mainLayout->addWidget(centerPanel, 1);
    mainLayout->addWidget(rightPanel);
    
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    
    // ========== CONNECT CANVAS SIGNALS ==========
    connect(m_canvas, &Canvas::componentAdded, this, &MainWindow::onComponentAdded);
    connect(m_canvas, &Canvas::componentLoaded, this, &MainWindow::onComponentLoaded);
    connect(m_canvas, &Canvas::designSubComponentAdded, this, &MainWindow::onDesignSubComponentAdded);
    connect(m_canvas, &Canvas::dropRejected, this, &MainWindow::onDropRejected);
    connect(m_canvas, &Canvas::modeChanged, this, &MainWindow::onModeChanged);
    
    // ========== INITIALIZE RUNTIME SERVICES ==========
    // Voice alert manager
    m_voiceAlertManager = new VoiceAlertManager(this);
    
    // Message server for health data from external systems
    m_messageServer = new MessageServer(this);
    if (m_messageServer->startServer(12345)) {
        m_statusLabel->setText("STATUS: ACTIVE  |  PORT: 12345  |  CLIENTS: 0");
    } else {
        m_statusLabel->setText("STATUS: SERVER FAILED");
        qWarning() << "[MainWindow] Failed to start message server on port 12345";
    }
    
    connect(m_messageServer, &MessageServer::messageReceived, 
            this, &MainWindow::onMessageReceived);
    connect(m_messageServer, &MessageServer::clientConnected,
            this, &MainWindow::onClientConnected);
    connect(m_messageServer, &MessageServer::clientDisconnected,
            this, &MainWindow::onClientDisconnected);
}

// ======================================================================
// Component Tabs (Enlarged Views)
// ======================================================================

void MainWindow::createComponentTabs()
{
    // Designer sees only System Overview canvas; User sees only monitor view.
    // Neither role gets per-component enlarged tabs.
    if (m_role == UserRole::Designer || m_role == UserRole::User) {
        return;
    }
    
    clearComponentTabs();
    
    QList<Component*> components = m_canvas->getComponents();
    qDebug() << "[MainWindow] Creating enlarged tabs for" << components.size() << "components";
    
    for (Component* comp : components) {
        addComponentTab(comp);
    }
}

void MainWindow::addComponentTab(Component* comp)
{
    // No per-component tabs for Designer or User roles
    if (m_role == UserRole::Designer || m_role == UserRole::User) {
        return;
    }
    
    if (!comp || !m_tabWidget) return;
    
    QString id = comp->getId();
    
    // Skip if tab already exists
    if (m_enlargedViews.contains(id)) return;
    
    QString typeId = comp->getTypeId();
    
    QStringList subNames;
    for (SubComponent* sub : comp->getSubComponents()) {
        subNames.append(sub->getName());
    }
    
    EnlargedComponentView* view = new EnlargedComponentView(id, typeId, subNames, m_tabWidget);
    m_enlargedViews[id] = view;
    
    view->updateComponentHealth(comp->getColor(), comp->getSize());
    
    QString displayName = comp->getDisplayName();
    QString tabName = "  " + displayName + "  ";
    m_tabWidget->addTab(view, tabName);
    
    qDebug() << "[MainWindow] Added enlarged tab for" << id << "(" << displayName << ")";
}

void MainWindow::clearComponentTabs()
{
    while (m_tabWidget && m_tabWidget->count() > 1) {
        QWidget* w = m_tabWidget->widget(m_tabWidget->count() - 1);
        m_tabWidget->removeTab(m_tabWidget->count() - 1);
        delete w;
    }
    m_enlargedViews.clear();
}

// ======================================================================
// Design Actions
// ======================================================================

void MainWindow::saveDesign()
{
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
    
    // Update analytics for all loaded components
    foreach (Component* comp, m_canvas->getComponents()) {
        onComponentAdded(comp->getId(), comp->getTypeId());
    }
    
    // Create enlarged component tabs
    createComponentTabs();
    
    QMessageBox::information(this, "Success", 
        "Design loaded!\nComponent tabs created for enlarged views.\n"
        "Health updates will appear when external systems connect.");
}

void MainWindow::clearCanvas()
{
    m_canvas->clearCanvas();
    m_analytics->clear();
    clearComponentTabs();
}

void MainWindow::addNewComponentType()
{
    AddComponentDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        ComponentDefinition def = dialog.getComponentDefinition();
        
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
    
    // Update analytics
    foreach (Component* comp, m_canvas->getComponents()) {
        onComponentAdded(comp->getId(), comp->getTypeId());
    }
    
    // Create enlarged component tabs
    createComponentTabs();
    
    qDebug() << "[MainWindow] Auto-loaded design from:" << foundPath;
    if (m_statusLabel) {
        m_statusLabel->setText(QString("STATUS: ACTIVE  |  PORT: 12345  |  CLIENTS: %1  |  DESIGN LOADED")
            .arg(m_connectedClients));
    }
}

// ======================================================================
// Signal Handlers
// ======================================================================

void MainWindow::onComponentAdded(const QString& id, const QString& typeId)
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    QString displayName = typeId;
    if (registry.hasComponent(typeId)) {
        displayName = registry.getComponent(typeId).displayName;
    }
    m_analytics->addComponent(id, displayName);
    
    // Create enlarged tab for the new component
    Component* comp = m_canvas->getComponentById(id);
    if (comp) {
        addComponentTab(comp);
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

void MainWindow::onDesignSubComponentAdded(const QString& parentId, SubComponentType subType)
{
    m_analytics->addDesignSubComponent(parentId, DesignSubComponent::typeToString(subType));
}

void MainWindow::onDropRejected(const QString& reason)
{
    QMessageBox::warning(this, "Invalid Drop", reason);
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
    
    // Update enlarged view if it exists
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

// ======================================================================
// Connection Mode
// ======================================================================

void MainWindow::toggleConnectionMode()
{
    if (!m_canvas || !m_connectBtn) return;
    
    if (m_connectBtn->isChecked()) {
        m_canvas->setMode(CanvasMode::Connect);
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

// ======================================================================
// Voice Alerts
// ======================================================================

void MainWindow::toggleVoiceAlerts()
{
    if (!m_voiceAlertManager || !m_voiceToggleBtn) return;
    
    bool isOn = m_voiceToggleBtn->isChecked();
    m_voiceAlertManager->setMuted(!isOn);
    m_voiceToggleBtn->setText(isOn ? "VOICE: ON" : "VOICE: OFF");
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

// ======================================================================
// Theme
// ======================================================================

void MainWindow::onThemeToggle()
{
    ThemeManager::instance().toggleTheme();
}

void MainWindow::onThemeChanged(AppTheme theme)
{
    Q_UNUSED(theme);
    updateThemeButtonText();
    refreshCanvasBackground();
    
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
    
    if (m_canvas->scene()) {
        m_canvas->scene()->update();
    }
}
