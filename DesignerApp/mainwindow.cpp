#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_componentList(nullptr)
    , m_canvas(nullptr)
    , m_analytics(nullptr)
{
    setupUI();
    setWindowTitle("Radar System Designer - Subsystem Layout Editor");
    resize(1200, 700);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // Create toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    
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
    
    QLabel* hintLabel = new QLabel(
        "Drag radar subsystems onto the canvas, then drop Label / LineEdit / Button sub-components inside them",
        centerPanel);
    hintLabel->setProperty("hint", true);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);
    
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
    connect(m_canvas, &Canvas::componentAdded,
            this, &MainWindow::onComponentAdded);
    connect(m_canvas, &Canvas::subComponentAdded,
            this, &MainWindow::onSubComponentAdded);
    connect(m_canvas, &Canvas::dropRejected,
            this, &MainWindow::onDropRejected);
}

void MainWindow::saveDesign()
{
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
    
    // Update analytics with loaded components and sub-components
    foreach (Component* comp, m_canvas->getComponents()) {
        onComponentAdded(comp->getId(), comp->getType());
        foreach (SubComponent* sub, comp->getSubComponents()) {
            m_analytics->addSubComponent(comp->getId(),
                                         SubComponent::typeToString(sub->getType()));
        }
    }
    
    QMessageBox::information(this, "Success", "Design loaded successfully!");
}

void MainWindow::clearCanvas()
{
    m_canvas->clearCanvas();
    m_analytics->clear();
}

void MainWindow::onComponentAdded(const QString& id, ComponentType type)
{
    m_analytics->addComponent(id, getComponentTypeName(type));
}

void MainWindow::onSubComponentAdded(const QString& parentId, SubComponentType subType)
{
    m_analytics->addSubComponent(parentId, SubComponent::typeToString(subType));
}

void MainWindow::onDropRejected(const QString& reason)
{
    QMessageBox::warning(this, "Invalid Drop", reason);
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
