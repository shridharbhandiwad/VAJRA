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
    , m_analyticsDashboard(nullptr)
    , m_messageServer(nullptr)
    , m_statusLabel(nullptr)
    , m_userLabel(nullptr)
    , m_connectedClients(0)
    , m_voiceAlertManager(nullptr)
    , m_voiceToggleBtn(nullptr)
    , m_connectBtn(nullptr)
    , m_connectionTypeCombo(nullptr)
    , m_themeToggleBtn(nullptr)
    , m_logoutBtn(nullptr)
    , m_saveBtn(nullptr)
    , m_loadBtn(nullptr)
    , m_clearBtn(nullptr)
    , m_addTypeBtn(nullptr)
    , m_importComponentBtn(nullptr)
    , m_importSubcomponentsBtn(nullptr)
    , m_saveBtnAction(nullptr)
    , m_loadBtnAction(nullptr)
    , m_clearBtnAction(nullptr)
    , m_addTypeBtnAction(nullptr)
    , m_importComponentBtnAction(nullptr)
    , m_importSubcomponentsBtnAction(nullptr)
    , m_connectBtnAction(nullptr)
    , m_connectionTypeComboAction(nullptr)
    , m_voiceToggleBtnAction(nullptr)
    , m_testVoiceBtnAction(nullptr)
    , m_designSep1(nullptr)
    , m_designSep2(nullptr)
    , m_connectSep(nullptr)
    , m_statusSep(nullptr)
    , m_voiceSep(nullptr)
    , m_leftPanel(nullptr)
    , m_tabWidget(nullptr)
{
    setupUI();
    
    // Set window title based on role
    QString roleStr = (m_role == UserRole::Designer) ? "Designer" : "Monitor";
    setWindowTitle(QString("Radar System - %1").arg(roleStr));
    
    // Connect to theme changes for live updates
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);
    
    // Apply role-based restrictions
    applyRoleRestrictions();
    
    // Auto-load design if available
    autoLoadDesign();
    
    // Open in maximized mode
    showMaximized();
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
        // Designer: Full design capabilities on System Overview canvas only.
        // No per-component enlarged view tabs (design-focused workflow).
        // Dashboard view is only for User mode.
        
        // Hide voice controls (not needed for design view)
        if (m_voiceToggleBtnAction) m_voiceToggleBtnAction->setVisible(false);
        if (m_testVoiceBtnAction) m_testVoiceBtnAction->setVisible(false);
        if (m_voiceSep) m_voiceSep->setVisible(false);
        
        // Hide dashboard button (only for User mode)
        if (m_dashboardBtn) m_dashboardBtn->setVisible(false);
        
    } else if (m_role == UserRole::User) {
        // User: Monitor-only. Can load designs to view, but no design tools.
        // Only visible toolbar items: User label, LOAD DESIGN, STATUS, Theme toggle, VIEW DASHBOARD.
        
        // Hide the left panel (components list)
        if (m_leftPanel) {
            m_leftPanel->setVisible(false);
        }
        
        // Hide design-only toolbar actions (must hide QAction, not just widget,
        // for QToolBar to properly collapse the space).
        // Keep Load Design visible for loading designs to view.
        if (m_saveBtnAction)   m_saveBtnAction->setVisible(false);
        if (m_clearBtnAction)  m_clearBtnAction->setVisible(false);
        if (m_addTypeBtnAction) m_addTypeBtnAction->setVisible(false);
        if (m_importComponentBtnAction) m_importComponentBtnAction->setVisible(false);
        if (m_importSubcomponentsBtnAction) m_importSubcomponentsBtnAction->setVisible(false);
        
        // Hide connection mode controls (design feature)
        if (m_connectBtnAction) m_connectBtnAction->setVisible(false);
        if (m_connectionTypeComboAction) m_connectionTypeComboAction->setVisible(false);
        
        // Hide voice controls (not needed for monitor-only view)
        if (m_voiceToggleBtnAction) m_voiceToggleBtnAction->setVisible(false);
        if (m_testVoiceBtnAction) m_testVoiceBtnAction->setVisible(false);
        
        // Hide separators between hidden items to keep toolbar clean
        if (m_designSep1)  m_designSep1->setVisible(false);
        if (m_designSep2)  m_designSep2->setVisible(false);
        if (m_connectSep)  m_connectSep->setVisible(false);
        if (m_statusSep)   m_statusSep->setVisible(false);
        if (m_voiceSep)    m_voiceSep->setVisible(false);
        
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
    
    m_importComponentBtn = new QPushButton("📥 IMPORT COMPONENT", this);
    m_importComponentBtn->setObjectName("importButton");
    m_importComponentBtn->setToolTip("Import a component from .cmp file");
    
    m_importSubcomponentsBtn = new QPushButton("📥 IMPORT WIDGETS", this);
    m_importSubcomponentsBtn->setObjectName("importButton");
    m_importSubcomponentsBtn->setToolTip("Import design widgets from .subcmp file into selected component");
    
    m_saveBtnAction = toolbar->addWidget(m_saveBtn);
    m_loadBtnAction = toolbar->addWidget(m_loadBtn);
    m_clearBtnAction = toolbar->addWidget(m_clearBtn);
    m_designSep1 = toolbar->addSeparator();
    m_addTypeBtnAction = toolbar->addWidget(m_addTypeBtn);
    m_importComponentBtnAction = toolbar->addWidget(m_importComponentBtn);
    m_importSubcomponentsBtnAction = toolbar->addWidget(m_importSubcomponentsBtn);
    m_designSep2 = toolbar->addSeparator();
    
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
    
    m_connectBtnAction = toolbar->addWidget(m_connectBtn);
    m_connectionTypeComboAction = toolbar->addWidget(m_connectionTypeCombo);
    m_connectSep = toolbar->addSeparator();
    
    // Runtime status & voice controls
    m_statusLabel = new QLabel("STATUS: INITIALIZING", this);
    m_statusLabel->setObjectName("statusLabel");
    toolbar->addWidget(m_statusLabel);
    m_statusSep = toolbar->addSeparator();
    
    m_voiceToggleBtn = new QPushButton("VOICE: ON", this);
    m_voiceToggleBtn->setObjectName("voiceToggleBtn");
    m_voiceToggleBtn->setToolTip("Toggle voice-based health status alerts");
    m_voiceToggleBtn->setCheckable(true);
    m_voiceToggleBtn->setChecked(true);
    
    QPushButton* testVoiceBtn = new QPushButton("TEST VOICE", this);
    testVoiceBtn->setObjectName("testVoiceBtn");
    testVoiceBtn->setToolTip("Test voice output");
    
    m_voiceToggleBtnAction = toolbar->addWidget(m_voiceToggleBtn);
    m_testVoiceBtnAction = toolbar->addWidget(testVoiceBtn);
    m_voiceSep = toolbar->addSeparator();
    
    // Theme toggle button
    m_themeToggleBtn = new QPushButton(this);
    m_themeToggleBtn->setObjectName("themeToggleBtn");
    m_themeToggleBtn->setCursor(Qt::PointingHandCursor);
    m_themeToggleBtn->setToolTip("Switch between Dark and Light themes");
    updateThemeButtonText();
    toolbar->addWidget(m_themeToggleBtn);
    
    toolbar->addSeparator();
    
    // Dashboard button
    m_dashboardBtn = new QPushButton("📊 VIEW DASHBOARD", this);
    m_dashboardBtn->setObjectName("dashboardButton");
    m_dashboardBtn->setCursor(Qt::PointingHandCursor);
    m_dashboardBtn->setToolTip("Open Advanced Analytics Dashboard");
    toolbar->addWidget(m_dashboardBtn);
    
    toolbar->addSeparator();
    
    // Logout button (top right corner)
    m_logoutBtn = new QPushButton("LOG OFF", this);
    m_logoutBtn->setObjectName("logoutButton");
    m_logoutBtn->setCursor(Qt::PointingHandCursor);
    m_logoutBtn->setToolTip("Log off and switch user/role");
    toolbar->addWidget(m_logoutBtn);
    
    // Connect toolbar signals
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::saveDesign);
    connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::loadDesign);
    connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::clearCanvas);
    connect(m_addTypeBtn, &QPushButton::clicked, this, &MainWindow::addNewComponentType);
    connect(m_importComponentBtn, &QPushButton::clicked, this, &MainWindow::importComponent);
    connect(m_importSubcomponentsBtn, &QPushButton::clicked, this, &MainWindow::importSubcomponents);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnectionMode);
    connect(m_connectionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onConnectionTypeChanged);
    connect(m_voiceToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleVoiceAlerts);
    connect(testVoiceBtn, &QPushButton::clicked, this, &MainWindow::testVoice);
    connect(m_themeToggleBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggle);
    connect(m_dashboardBtn, &QPushButton::clicked, this, &MainWindow::showAnalyticsDashboard);
    connect(m_logoutBtn, &QPushButton::clicked, this, &MainWindow::onLogout);
    
    // ========== MAIN LAYOUT ==========
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    
    // ========== LEFT PANEL - Components & Sub-Components List ==========
    m_leftPanel = new QWidget(this);
    m_leftPanel->setObjectName("leftPanel");
    QVBoxLayout* leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setSpacing(4);
    leftLayout->setContentsMargins(6, 6, 6, 6);
    
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
    
    leftLayout->addWidget(componentsLabel);
    leftLayout->addWidget(countLabel);
    leftLayout->addWidget(m_componentList);
    leftLayout->addWidget(addInlineBtn);
    m_leftPanel->setLayout(leftLayout);
    m_leftPanel->setMaximumWidth(280);
    m_leftPanel->setMinimumWidth(220);
    
    connect(&registry, &ComponentRegistry::registryChanged, this, [countLabel, &registry]() {
        countLabel->setText(QString("%1 types available").arg(registry.componentCount()));
    });
    
    // ========== CENTER PANEL - Tab Widget (Canvas + Enlarged Views) ==========
    QWidget* centerPanel = new QWidget(this);
    centerPanel->setObjectName("centerPanel");
    QVBoxLayout* centerLayout = new QVBoxLayout(centerPanel);
    centerLayout->setSpacing(0);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    
    m_tabWidget = new QTabWidget(centerPanel);
    m_tabWidget->setObjectName("componentTabWidget");
    m_tabWidget->setDocumentMode(false);
    m_tabWidget->setTabPosition(QTabWidget::North);
    
    // System Overview tab with canvas
    QWidget* overviewTab = new QWidget();
    overviewTab->setObjectName("overviewTab");
    QVBoxLayout* overviewLayout = new QVBoxLayout(overviewTab);
    overviewLayout->setSpacing(2);
    overviewLayout->setContentsMargins(2, 2, 2, 2);
    
    m_canvas = new Canvas(overviewTab);
    m_canvas->setObjectName("mainCanvas");
    m_canvas->setUserRole(m_role);
    
    overviewLayout->addWidget(m_canvas);
    
    m_tabWidget->addTab(overviewTab, "  System Overview  ");
    
    centerLayout->addWidget(m_tabWidget);
    centerPanel->setLayout(centerLayout);
    
    // ========== RIGHT PANEL - System Overview ==========
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(4);
    rightLayout->setContentsMargins(6, 6, 6, 6);
    
    QLabel* analyticsLabel = new QLabel("SYSTEM OVERVIEW", rightPanel);
    analyticsLabel->setProperty("heading", true);
    
    m_analytics = new Analytics(rightPanel);
    m_analytics->setObjectName("analyticsPanel");
    
    rightLayout->addWidget(analyticsLabel);
    rightLayout->addWidget(m_analytics);
    rightPanel->setLayout(rightLayout);
    rightPanel->setMaximumWidth(260);
    rightPanel->setMinimumWidth(200);
    
    // ========== ASSEMBLE MAIN LAYOUT ==========
    mainLayout->addWidget(m_leftPanel);
    mainLayout->addWidget(centerPanel, 1);
    mainLayout->addWidget(rightPanel);
    
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);
    
    // ========== CONNECT CANVAS SIGNALS ==========
    connect(m_canvas, &Canvas::componentAdded, this, &MainWindow::onComponentAdded);
    connect(m_canvas, &Canvas::componentLoaded, this, &MainWindow::onComponentLoaded);
    connect(m_canvas, &Canvas::componentEdited, this, &MainWindow::onComponentEdited);
    connect(m_canvas, &Canvas::componentRemoved, this, &MainWindow::onComponentRemoved);
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
    connect(m_messageServer, &MessageServer::subsystemHealthReceived,
            this, &MainWindow::onSubsystemHealthReceived);
    connect(m_messageServer, &MessageServer::telemetryReceived,
            this, &MainWindow::onTelemetryReceived);
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
    // Per-component enlarged view tabs are only for the User (monitor) role
    if (m_role != UserRole::User) {
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
    // Per-component enlarged view tabs are only for the User (monitor) role
    if (m_role != UserRole::User) {
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
    
    // Get the actual display name from the component (may be customized)
    QString displayName = comp->getDisplayName();
    
    // Update the view with the actual component data (display name and label)
    view->updateFromComponent(comp);
    
    view->updateComponentHealth(comp->getColor(), comp->getSize());
    
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

void MainWindow::importComponent()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Import Component",
                                                    "",
                                                    "Component Files (*.cmp);;All Files (*)");
    if (fileName.isEmpty()) return;
    
    if (m_canvas && m_canvas->importComponent(fileName)) {
        QMessageBox::information(this, "Import Successful",
                                QString("Component imported from:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, "Import Failed",
                            QString("Could not import component from:\n%1").arg(fileName));
    }
}

void MainWindow::importSubcomponents()
{
    // First, get the selected component
    Component* selectedComponent = nullptr;
    QList<QGraphicsItem*> selectedItems = m_canvas->scene()->selectedItems();
    for (QGraphicsItem* item : selectedItems) {
        Component* comp = dynamic_cast<Component*>(item);
        if (comp) {
            selectedComponent = comp;
            break;
        }
    }
    
    if (!selectedComponent) {
        QMessageBox::information(this, "No Component Selected",
                                "Please select a component on the canvas first.\n\n"
                                "The imported widgets will be added to the selected component.");
        return;
    }
    
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "Import Design Widgets",
                                                    "",
                                                    "Subcomponent Files (*.subcmp);;All Files (*)");
    if (fileName.isEmpty()) return;
    
    if (m_canvas && m_canvas->importSubcomponents(fileName, selectedComponent)) {
        QMessageBox::information(this, "Import Successful",
                                QString("Design widgets imported from:\n%1\n\n"
                                       "into component: %2")
                                    .arg(fileName, selectedComponent->getDisplayName()));
    } else {
        QMessageBox::warning(this, "Import Failed",
                            QString("Could not import widgets from:\n%1").arg(fileName));
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
    
    // Add all subcomponents to analytics
    Component* comp = m_canvas->getComponentById(id);
    if (comp) {
        // Add regular SubComponents
        for (SubComponent* sub : comp->getSubComponents()) {
            m_analytics->addSubComponent(id, sub->getName());
        }
        
        // Add DesignSubComponents
        for (DesignSubComponent* dsub : comp->getDesignSubComponents()) {
            m_analytics->addDesignSubComponent(id, DesignSubComponent::typeToString(dsub->getType()));
        }
        
        // Create enlarged tab for the new component
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
    
    // Add all subcomponents to analytics
    Component* comp = m_canvas->getComponentById(id);
    if (comp) {
        // Add regular SubComponents
        for (SubComponent* sub : comp->getSubComponents()) {
            m_analytics->addSubComponent(id, sub->getName());
        }
        
        // Add DesignSubComponents
        for (DesignSubComponent* dsub : comp->getDesignSubComponents()) {
            m_analytics->addDesignSubComponent(id, DesignSubComponent::typeToString(dsub->getType()));
        }
    }
}

void MainWindow::onComponentEdited(const QString& id, const QString& /*typeId*/)
{
    Component* comp = m_canvas->getComponentById(id);
    if (!comp) {
        qWarning() << "[MainWindow] Component" << id << "not found during edit callback";
        return;
    }
    
    // Get the actual display name from the component (may be customized)
    QString displayName = comp->getDisplayName();
    
    // Refresh the component in analytics (clears old subcomponents)
    m_analytics->refreshComponent(id, displayName);
    
    // Re-add all subcomponents to analytics
    // Add regular SubComponents
    for (SubComponent* sub : comp->getSubComponents()) {
        m_analytics->addSubComponent(id, sub->getName());
    }
    
    // Add DesignSubComponents
    for (DesignSubComponent* dsub : comp->getDesignSubComponents()) {
        m_analytics->addDesignSubComponent(id, DesignSubComponent::typeToString(dsub->getType()));
    }
    
    // Update the enlarged view tab if it exists
    if (m_enlargedViews.contains(id)) {
        EnlargedComponentView* view = m_enlargedViews[id];
        
        // Update the view with the updated component data
        view->updateFromComponent(comp);
        
        // Update the tab name
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (m_tabWidget->widget(i) == view) {
                QString tabName = "  " + displayName + "  ";
                m_tabWidget->setTabText(i, tabName);
                break;
            }
        }
        
        qDebug() << "[MainWindow] Updated enlarged view tab for" << id << "with new display name:" << displayName;
    }
    
    qDebug() << "[MainWindow] Component" << id << "edited and analytics refreshed";
}

void MainWindow::onComponentRemoved(const QString& id, const QString& typeId)
{
    Q_UNUSED(typeId);
    
    qDebug() << "[MainWindow] Component removed:" << id;
    
    // Remove from analytics
    m_analytics->removeComponent(id);
    
    // Close and remove enlarged view tab if it exists
    if (m_enlargedViews.contains(id)) {
        EnlargedComponentView* view = m_enlargedViews[id];
        
        // Find and remove the tab
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (m_tabWidget->widget(i) == view) {
                m_tabWidget->removeTab(i);
                break;
            }
        }
        
        m_enlargedViews.remove(id);
        delete view;
    }
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

void MainWindow::onSubsystemHealthReceived(const QString& componentId,
                                            const QString& subsystemName,
                                            const QString& color, qreal health)
{
    // Update the sub-component within the canvas component
    Component* comp = m_canvas->getComponentById(componentId);
    if (comp) {
        SubComponent* sub = comp->getSubComponent(subsystemName);
        if (sub) {
            sub->setHealth(health);
            sub->setColor(QColor(color));
        }
    }
    
    // Update the enlarged view if it exists
    if (m_enlargedViews.contains(componentId)) {
        m_enlargedViews[componentId]->updateSubcomponentHealth(subsystemName, health, QColor(color));
    }
}

void MainWindow::onTelemetryReceived(const QString& componentId, const QJsonObject& telemetry)
{
    Q_UNUSED(telemetry);
    // Full APCU telemetry received - log for now, can be extended
    // for detailed quadrant/channel views in the future
    qDebug() << "[MainWindow] Full APCU telemetry received for" << componentId
             << "- unit:" << telemetry.value("unit").toString()
             << "- array_voltage:" << telemetry.value("array_voltage").toDouble()
             << "- array_current:" << telemetry.value("array_current").toDouble();
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

// ======================================================================
// Analytics Dashboard
// ======================================================================

void MainWindow::showAnalyticsDashboard()
{
    // Create dashboard if it doesn't exist
    if (!m_analyticsDashboard) {
        m_analyticsDashboard = new AnalyticsDashboard(this);
        
        // Populate dashboard with current component data
        const QList<Component*>& components = m_canvas->getComponents();
        for (Component* comp : components) {
            if (comp) {
                m_analyticsDashboard->addComponent(comp->getId(), comp->getTypeId());
            }
        }
        
        // Note: If you want to sync real-time data, you can connect signals here
        // For now, the dashboard uses generated sample data
    }
    
    // Show the dashboard
    m_analyticsDashboard->show();
    m_analyticsDashboard->raise();
    m_analyticsDashboard->activateWindow();
}

// ======================================================================
// Logout
// ======================================================================

void MainWindow::onLogout()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Confirm Logout");
    msgBox.setText("Are you sure you want to log off?");
    msgBox.setInformativeText("You will be returned to the login screen to switch users or roles.");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setIcon(QMessageBox::Question);
    
    if (msgBox.exec() == QMessageBox::Yes) {
        emit logoutRequested();
        close();
    }
}
