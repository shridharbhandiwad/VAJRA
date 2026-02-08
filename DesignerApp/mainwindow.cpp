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
    setWindowTitle("Designer Application - Component Editor");
    resize(1200, 700);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // Create toolbar
    QToolBar* toolbar = addToolBar("Main Toolbar");
    
    QPushButton* saveBtn = new QPushButton("Save Design", this);
    QPushButton* loadBtn = new QPushButton("Load Design", this);
    QPushButton* clearBtn = new QPushButton("Clear Canvas", this);
    
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
    
    QLabel* componentsLabel = new QLabel("All Components", leftPanel);
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
    
    QLabel* hintLabel = new QLabel("Drag and drop canvas", centerPanel);
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
    
    // Update analytics with loaded components
    foreach (Component* comp, m_canvas->getComponents()) {
        onComponentAdded(comp->getId(), comp->getType());
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

QString MainWindow::getComponentTypeName(ComponentType type)
{
    switch (type) {
        case ComponentType::Circle: return "Circle";
        case ComponentType::Square: return "Square";
        case ComponentType::Triangle: return "Triangle";
        default: return "Unknown";
    }
}
