#include "analyticsdashboard.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QDateTime>
#include <QRandomGenerator>
#include <QEasingCurve>
#include <QtCharts/QChart>
#include <QtCharts/QLegend>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrinter>
#include <QPainter>
#include <QPixmap>

AnalyticsDashboard::AnalyticsDashboard(QWidget* parent)
    : QMainWindow(parent)
    , m_scrollArea(nullptr)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_totalComponentsLabel(nullptr)
    , m_activeComponentsLabel(nullptr)
    , m_avgHealthLabel(nullptr)
    , m_totalAlertsLabel(nullptr)
    , m_timeRangeCombo(nullptr)
    , m_componentFilterCombo(nullptr)
    , m_refreshBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_updateTimer(nullptr)
{
    setObjectName("AnalyticsDashboard");
    setWindowTitle("DATA ANALYTICS DASHBOARD");
    resize(1400, 850);
    
    // Initialize chart grids
    for (int i = 0; i < 4; i++) {
        m_chartGrids[i].chartView = nullptr;
        m_chartGrids[i].chartTypeCombo = nullptr;
        m_chartGrids[i].containerWidget = nullptr;
    }
    
    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &AnalyticsDashboard::onThemeChanged);
    
    setupUI();
    // Don't generate sample data - wait for real health messages from external system
    // generateSampleData();
    updateKPIs();
    updateAllCharts();
    
    // Setup auto-refresh timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &AnalyticsDashboard::updateAllCharts);
    m_updateTimer->start(3000); // Update every 3 seconds
}

AnalyticsDashboard::~AnalyticsDashboard()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void AnalyticsDashboard::setupUI()
{
    // Apply theme colors
    ThemeManager& tm = ThemeManager::instance();
    
    m_bgColor = tm.windowBackground();
    m_textColor = tm.primaryText();
    m_gridColor = tm.chartGridLine();
    m_chartBgColor = tm.chartBackground();
    
    // Create scroll area for dashboard
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("dashboardScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    
    m_centralWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(12);
    m_mainLayout->setContentsMargins(16, 12, 16, 16);
    
    // ========== HEADER SECTION WITH ENHANCED STYLING ==========
    QWidget* headerWidget = new QWidget();
    headerWidget->setObjectName("dashboardHeader");
    QVBoxLayout* headerMainLayout = new QVBoxLayout(headerWidget);
    headerMainLayout->setSpacing(8);
    headerMainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Title row with icon-like decoration
    QHBoxLayout* titleRow = new QHBoxLayout();
    titleRow->setSpacing(12);
    
    QLabel* titleLabel = new QLabel("ADVANCED DATA ANALYTICS");
    titleLabel->setObjectName("dashboardTitle");
    QFont titleFont;
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleFont.setWeight(QFont::ExtraBold);
    titleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    titleLabel->setFont(titleFont);
    
    titleRow->addWidget(titleLabel);
    titleRow->addStretch();
    
    // Add subtitle/description
    QLabel* subtitleLabel = new QLabel("Real-time system monitoring and performance analytics");
    subtitleLabel->setObjectName("dashboardSubtitle");
    QFont subtitleFont;
    subtitleFont.setPointSize(9);
    subtitleFont.setWeight(QFont::Normal);
    subtitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    subtitleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.2);
    subtitleLabel->setFont(subtitleFont);
    
    // Controls row with better spacing and styling
    QHBoxLayout* controlsRow = new QHBoxLayout();
    controlsRow->setSpacing(12);
    
    QLabel* filterLabel = new QLabel("FILTER:");
    filterLabel->setObjectName("filterLabel");
    QFont labelFont;
    labelFont.setPointSize(9);
    labelFont.setBold(true);
    labelFont.setWeight(QFont::Bold);
    labelFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    labelFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    filterLabel->setFont(labelFont);
    
    m_componentFilterCombo = new QComboBox();
    m_componentFilterCombo->setObjectName("dashboardCombo");
    m_componentFilterCombo->addItem("ALL COMPONENTS");
    m_componentFilterCombo->setFixedWidth(180);
    QFont comboFont;
    comboFont.setPointSize(9);
    comboFont.setWeight(QFont::Medium);
    comboFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    m_componentFilterCombo->setFont(comboFont);
    connect(m_componentFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AnalyticsDashboard::onComponentFilterChanged);
    
    QLabel* timeLabel = new QLabel("TIMEFRAME:");
    timeLabel->setObjectName("filterLabel");
    timeLabel->setFont(labelFont);
    
    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->setObjectName("dashboardCombo");
    m_timeRangeCombo->addItems({"Last Hour", "Last 6 Hours", "Last 24 Hours", "Last Week"});
    m_timeRangeCombo->setCurrentIndex(2);
    m_timeRangeCombo->setFixedWidth(140);
    m_timeRangeCombo->setFont(comboFont);
    
    controlsRow->addWidget(filterLabel);
    controlsRow->addWidget(m_componentFilterCombo);
    controlsRow->addWidget(timeLabel);
    controlsRow->addWidget(m_timeRangeCombo);
    controlsRow->addStretch();
    
    m_refreshBtn = new QPushButton("REFRESH");
    m_refreshBtn->setObjectName("dashboardRefreshBtn");
    m_refreshBtn->setMinimumWidth(90);
    m_refreshBtn->setFixedHeight(32);
    QFont btnFont;
    btnFont.setPointSize(9);
    btnFont.setBold(true);
    btnFont.setWeight(QFont::Bold);
    btnFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    btnFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    m_refreshBtn->setFont(btnFont);
    connect(m_refreshBtn, &QPushButton::clicked, this, &AnalyticsDashboard::refreshDashboard);
    
    m_exportBtn = new QPushButton("EXPORT PDF");
    m_exportBtn->setObjectName("dashboardExportBtn");
    m_exportBtn->setMinimumWidth(100);
    m_exportBtn->setFixedHeight(32);
    m_exportBtn->setFont(btnFont);
    connect(m_exportBtn, &QPushButton::clicked, this, &AnalyticsDashboard::onExportToPDF);
    
    controlsRow->addWidget(m_refreshBtn);
    controlsRow->addWidget(m_exportBtn);
    
    headerMainLayout->addLayout(titleRow);
    headerMainLayout->addWidget(subtitleLabel);
    headerMainLayout->addLayout(controlsRow);
    
    m_mainLayout->addWidget(headerWidget);
    
    // ========== ENHANCED KPI SECTION ==========
    m_mainLayout->addWidget(createKPISection());
    
    // Add elegant gradient divider
    QFrame* divider = new QFrame();
    divider->setObjectName("dashboardDivider");
    divider->setFrameShape(QFrame::HLine);
    divider->setFixedHeight(2);
    m_mainLayout->addWidget(divider);
    
    // ========== ENHANCED 2x2 CHARTS GRID ==========
    QGridLayout* chartsGrid = new QGridLayout();
    chartsGrid->setObjectName("chartsGrid");
    chartsGrid->setSpacing(12);
    chartsGrid->setContentsMargins(0, 4, 0, 0);
    
    // Create 2x2 grid with default chart types
    m_chartGrids[0].containerWidget = createChartGrid(0, ChartType::HealthTrend);
    m_chartGrids[1].containerWidget = createChartGrid(1, ChartType::ComponentDistribution);
    m_chartGrids[2].containerWidget = createChartGrid(2, ChartType::SubsystemPerformance);
    m_chartGrids[3].containerWidget = createChartGrid(3, ChartType::AlertHistory);
    
    // Add to grid layout (2x2)
    chartsGrid->addWidget(m_chartGrids[0].containerWidget, 0, 0);
    chartsGrid->addWidget(m_chartGrids[1].containerWidget, 0, 1);
    chartsGrid->addWidget(m_chartGrids[2].containerWidget, 1, 0);
    chartsGrid->addWidget(m_chartGrids[3].containerWidget, 1, 1);
    
    // Set equal stretch
    chartsGrid->setColumnStretch(0, 1);
    chartsGrid->setColumnStretch(1, 1);
    chartsGrid->setRowStretch(0, 1);
    chartsGrid->setRowStretch(1, 1);
    
    m_mainLayout->addLayout(chartsGrid);
    
    m_scrollArea->setWidget(m_centralWidget);
    setCentralWidget(m_scrollArea);
}

QWidget* AnalyticsDashboard::createKPISection()
{
    ThemeManager& tm = ThemeManager::instance();
    
    QWidget* kpiWidget = new QWidget();
    kpiWidget->setObjectName("kpiSection");
    QHBoxLayout* kpiLayout = new QHBoxLayout(kpiWidget);
    kpiLayout->setSpacing(12);
    kpiLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create KPI cards with enhanced visual design and varied colors
    QColor primaryBlue = QColor(41, 128, 185);    // Modern blue
    QColor successGreen = QColor(39, 174, 96);    // Vibrant green
    QColor warningOrange = QColor(243, 156, 18);  // Warm orange
    QColor dangerRed = QColor(231, 76, 60);       // Bold red
    
    QWidget* card1 = createKPICard("COMPONENTS", "0", "Total Monitored", primaryBlue);
    QWidget* card2 = createKPICard("ACTIVE", "0", "Systems Online", successGreen);
    QWidget* card3 = createKPICard("HEALTH", "0%", "Average Status", warningOrange);
    QWidget* card4 = createKPICard("ALERTS", "0", "Total Warnings", dangerRed);
    
    kpiLayout->addWidget(card1);
    kpiLayout->addWidget(card2);
    kpiLayout->addWidget(card3);
    kpiLayout->addWidget(card4);
    
    return kpiWidget;
}

QWidget* AnalyticsDashboard::createKPICard(const QString& title, const QString& value, const QString& subtitle, const QColor& color)
{
    QWidget* card = new QWidget();
    card->setObjectName("kpiCard");
    card->setMinimumHeight(90);
    card->setMaximumHeight(110);
    
    // Enhanced border with gradient effect using color
    QString gradientColor = color.lighter(110).name();
    card->setStyleSheet(QString(
        "QWidget#kpiCard { "
        "   border-left: 4px solid %1; "
        "   border-top: 1px solid %2; "
        "}"
    ).arg(color.name(), gradientColor));
    
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(4);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    
    // Title with icon
    QLabel* titleLabel = new QLabel(title);
    titleLabel->setObjectName("kpiTitle");
    QFont titleFont;
    titleFont.setPointSize(9);
    titleFont.setBold(true);
    titleFont.setWeight(QFont::Bold);
    titleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QString("color: %1;").arg(color.name()));
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    
    // Value with enhanced styling
    QLabel* valueLabel = new QLabel(value);
    valueLabel->setObjectName("kpiValue");
    QFont valueFont;
    valueFont.setPointSize(28);
    valueFont.setBold(true);
    valueFont.setWeight(QFont::Black);
    valueFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    valueFont.setLetterSpacing(QFont::AbsoluteSpacing, -1.5);
    valueLabel->setFont(valueFont);
    valueLabel->setAlignment(Qt::AlignCenter);
    
    // Store label for updates - check for key terms
    if (title.contains("COMPONENTS")) m_totalComponentsLabel = valueLabel;
    else if (title.contains("ACTIVE")) m_activeComponentsLabel = valueLabel;
    else if (title.contains("HEALTH")) m_avgHealthLabel = valueLabel;
    else if (title.contains("ALERTS")) m_totalAlertsLabel = valueLabel;
    
    // Subtitle with better styling
    QLabel* subtitleLabel = new QLabel(subtitle);
    subtitleLabel->setObjectName("kpiSubtitle");
    QFont subtitleFont;
    subtitleFont.setPointSize(8);
    subtitleFont.setWeight(QFont::Medium);
    subtitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    subtitleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    
    // Progress bar or indicator (visual enhancement)
    QFrame* progressBar = new QFrame();
    progressBar->setObjectName("kpiProgressBar");
    progressBar->setFixedHeight(3);
    progressBar->setStyleSheet(QString(
        "QFrame#kpiProgressBar { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "       stop:0 %1, stop:0.7 %2, stop:1 transparent); "
        "   border-radius: 2px; "
        "}"
    ).arg(color.name(), color.lighter(130).name()));
    
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(valueLabel, 1);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addWidget(progressBar);
    
    return card;
}

QWidget* AnalyticsDashboard::createChartGrid(int gridIndex, ChartType initialType)
{
    QWidget* container = new QWidget();
    container->setObjectName("chartContainer");
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);
    
    // Chart header with title and dropdown
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);
    
    // Create dropdown for chart type selection
    QComboBox* chartTypeCombo = new QComboBox();
    chartTypeCombo->setObjectName("chartTypeCombo");
    QFont comboFont;
    comboFont.setPointSize(9);
    comboFont.setBold(true);
    comboFont.setWeight(QFont::Bold);
    comboFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    comboFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
    chartTypeCombo->setFont(comboFont);
    chartTypeCombo->addItem("Health Trend", static_cast<int>(ChartType::HealthTrend));
    chartTypeCombo->addItem("Component Distribution", static_cast<int>(ChartType::ComponentDistribution));
    chartTypeCombo->addItem("Subsystem Performance", static_cast<int>(ChartType::SubsystemPerformance));
    chartTypeCombo->addItem("Message Frequency", static_cast<int>(ChartType::MessageFrequency));
    chartTypeCombo->addItem("Alert History", static_cast<int>(ChartType::AlertHistory));
    chartTypeCombo->addItem("Component Comparison", static_cast<int>(ChartType::ComponentComparison));
    
    // Set initial selection
    chartTypeCombo->setCurrentIndex(static_cast<int>(initialType));
    
    // Connect dropdown to update handler
    connect(chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, gridIndex](int index) {
                this->onChartTypeChanged(gridIndex);
            });
    
    headerLayout->addWidget(chartTypeCombo, 1);
    
    // Add subtle separator
    QFrame* separator = new QFrame();
    separator->setObjectName("chartSeparator");
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);
    
    // Create chart view with enhanced settings
    QChart* chart = new QChart();
    applyChartTheme(chart);
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRenderHint(QPainter::TextAntialiasing);
    chartView->setRenderHint(QPainter::SmoothPixmapTransform);
    chartView->setMinimumHeight(250);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Enable tooltips
    enableChartTooltips(chartView);
    
    layout->addLayout(headerLayout);
    layout->addWidget(separator);
    layout->addWidget(chartView, 1);
    
    // Store in grid structure
    m_chartGrids[gridIndex].chartView = chartView;
    m_chartGrids[gridIndex].chartTypeCombo = chartTypeCombo;
    m_chartGrids[gridIndex].currentChartType = initialType;
    m_chartGrids[gridIndex].containerWidget = container;
    
    return container;
}

void AnalyticsDashboard::onChartTypeChanged(int gridIndex)
{
    if (gridIndex < 0 || gridIndex >= 4) return;
    
    // Get selected chart type from combo box
    ChartType newType = static_cast<ChartType>(
        m_chartGrids[gridIndex].chartTypeCombo->currentData().toInt()
    );
    
    m_chartGrids[gridIndex].currentChartType = newType;
    
    // Update the chart
    updateChartGrid(gridIndex, newType);
}

void AnalyticsDashboard::onComponentFilterChanged(int index)
{
    // Refresh all charts with the new filter
    updateAllCharts();
}

void AnalyticsDashboard::updateChartGrid(int gridIndex, ChartType chartType)
{
    if (gridIndex < 0 || gridIndex >= 4) return;
    
    QChartView* chartView = m_chartGrids[gridIndex].chartView;
    if (!chartView) return;
    
    // Get current component filter
    QString componentFilter = m_componentFilterCombo->currentText();
    if (componentFilter == "ALL COMPONENTS") {
        componentFilter = "";
    }
    
    updateChart(chartView, chartType, componentFilter);
}

void AnalyticsDashboard::updateChart(QChartView* chartView, ChartType chartType, const QString& componentFilter)
{
    switch (chartType) {
        case ChartType::HealthTrend:
            updateHealthTrendChart(chartView, componentFilter);
            break;
        case ChartType::ComponentDistribution:
            updateComponentDistributionChart(chartView, componentFilter);
            break;
        case ChartType::SubsystemPerformance:
            updateSubsystemPerformanceChart(chartView, componentFilter);
            break;
        case ChartType::MessageFrequency:
            updateMessageFrequencyChart(chartView, componentFilter);
            break;
        case ChartType::AlertHistory:
            updateAlertHistoryChart(chartView, componentFilter);
            break;
        case ChartType::ComponentComparison:
            updateComponentComparisonChart(chartView, componentFilter);
            break;
    }
}

void AnalyticsDashboard::createCharts()
{
    // This method is no longer needed with the new architecture
    // Charts are created dynamically in createChartGrid
}

void AnalyticsDashboard::generateSampleData()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 hourMs = 3600000; // 1 hour in milliseconds
    
    // Generate sample components
    QStringList componentTypes = {"RADAR", "ANTENNA", "POWER_SYSTEM", "COMMUNICATION", "COOLING_UNIT"};
    QStringList subsystems = {"TRANSMITTER", "RECEIVER", "PROCESSOR"};
    
    QRandomGenerator* rng = QRandomGenerator::global();
    
    // Create 5-8 sample components
    int numComponents = 5 + rng->bounded(4);
    for (int i = 0; i < numComponents; i++) {
        QString componentId = QString("COMP_%1").arg(i + 1, 3, 10, QChar('0'));
        QString type = componentTypes[rng->bounded(componentTypes.size())];
        
        ComponentHealthData data;
        data.componentId = componentId;
        data.type = type;
        data.totalMessages = 0;
        data.alertCount = 0;
        data.lastUpdateTime = currentTime;
        
        // Generate health history - 60 points (last 1 hour, every minute)
        int numPoints = 60;
        qreal baseHealth = 70.0 + rng->bounded(25);
        
        for (int j = 0; j < numPoints; j++) {
            qint64 timestamp = currentTime - (hourMs) + (j * 60000); // Last 1 hour
            
            // Add some variation
            qreal healthVariation = (rng->bounded(1000) / 1000.0 - 0.5) * 10;
            qreal health = qBound(50.0, baseHealth + healthVariation, 100.0);
            
            // Occasionally drop health to simulate issues
            if (rng->bounded(100) < 3) {
                health -= 15 + rng->bounded(15);
                data.alertCount++;
            }
            
            data.healthHistory.append(qMakePair(timestamp, health));
            
            // Add subsystem health data
            for (const QString& subsys : subsystems) {
                qreal subsysHealth = health + (rng->bounded(1000) / 1000.0 - 0.5) * 10;
                subsysHealth = qBound(40.0, subsysHealth, 100.0);
                
                if (!data.subsystemHealth.contains(subsys)) {
                    data.subsystemHealth[subsys] = QVector<qreal>();
                }
                data.subsystemHealth[subsys].append(subsysHealth);
            }
            
            // Generate message timestamps
            if (rng->bounded(100) < 15) {
                data.messageTimestamps.append(timestamp);
                data.totalMessages++;
            }
        }
        
        // Set current health to last value
        if (!data.healthHistory.isEmpty()) {
            data.currentHealth = data.healthHistory.last().second;
            data.currentStatus = getHealthStatus(data.currentHealth);
        } else {
            data.currentHealth = baseHealth;
            data.currentStatus = getHealthStatus(baseHealth);
        }
        
        m_componentData[componentId] = data;
        m_componentTypeCount[type]++;
        
        // Add to filter combo
        m_componentFilterCombo->addItem(componentId);
    }
}

void AnalyticsDashboard::updateKPIs()
{
    if (m_componentData.isEmpty()) {
        return;
    }
    
    // Get component filter
    QString componentFilter = m_componentFilterCombo->currentText();
    if (componentFilter == "ALL COMPONENTS") {
        componentFilter = "";
    }
    
    // Calculate KPI values
    int totalComponents = 0;
    int activeComponents = 0;
    qreal totalHealth = 0.0;
    int totalAlerts = 0;
    
    for (const auto& data : m_componentData) {
        if (!componentFilter.isEmpty() && data.componentId != componentFilter) {
            continue;
        }
        
        totalComponents++;
        if (data.currentHealth > 50.0) {
            activeComponents++;
        }
        totalHealth += data.currentHealth;
        totalAlerts += data.alertCount;
    }
    
    qreal avgHealth = totalComponents > 0 ? totalHealth / totalComponents : 0.0;
    
    // Update KPI labels with professional formatting
    if (m_totalComponentsLabel) {
        m_totalComponentsLabel->setText(QString::number(totalComponents));
    }
    if (m_activeComponentsLabel) {
        m_activeComponentsLabel->setText(QString::number(activeComponents));
    }
    if (m_avgHealthLabel) {
        // Format health with one decimal place for precision
        m_avgHealthLabel->setText(QString("%1%").arg(avgHealth, 0, 'f', 1));
    }
    if (m_totalAlertsLabel) {
        m_totalAlertsLabel->setText(QString::number(totalAlerts));
    }
}

QColor AnalyticsDashboard::getHealthColor(qreal health)
{
    ThemeManager& tm = ThemeManager::instance();
    if (health >= 90) return tm.accentSuccess();
    if (health >= 75) return tm.accentPrimary();
    if (health >= 60) return tm.accentWarning();
    if (health >= 40) return tm.accentWarning().darker(120);
    return tm.accentDanger();
}

QString AnalyticsDashboard::getHealthStatus(qreal health)
{
    if (health >= 90) return "EXCELLENT";
    if (health >= 75) return "GOOD";
    if (health >= 60) return "FAIR";
    if (health >= 40) return "POOR";
    return "CRITICAL";
}

QVector<QColor> AnalyticsDashboard::getChartPalette() const
{
    // Modern, vibrant color palette inspired by modern analytics dashboards
    return QVector<QColor>{
        QColor(52, 152, 219),   // Bright Blue
        QColor(46, 204, 113),   // Emerald Green
        QColor(155, 89, 182),   // Amethyst Purple
        QColor(241, 196, 15),   // Sun Yellow
        QColor(231, 76, 60),    // Alizarin Red
        QColor(26, 188, 156),   // Turquoise
        QColor(230, 126, 34),   // Carrot Orange
        QColor(52, 73, 94),     // Wet Asphalt
        QColor(142, 68, 173),   // Wisteria
        QColor(22, 160, 133),   // Green Sea
        QColor(243, 156, 18),   // Orange
        QColor(189, 195, 199)   // Silver
    };
}

void AnalyticsDashboard::recordComponentHealth(const QString& componentId, const QString& color, qreal health, qint64 timestamp)
{
    if (!m_componentData.contains(componentId)) {
        ComponentHealthData data;
        data.componentId = componentId;
        data.type = "UNKNOWN";
        data.currentHealth = 0.0;  // Initialize to 0% until first message
        data.currentStatus = "UNKNOWN";
        data.totalMessages = 0;
        data.alertCount = 0;
        data.lastUpdateTime = timestamp;
        m_componentData[componentId] = data;
    }
    
    ComponentHealthData& data = m_componentData[componentId];
    data.healthHistory.append(qMakePair(timestamp, health));
    data.currentHealth = health;
    data.currentStatus = getHealthStatus(health);
    data.lastUpdateTime = timestamp;
    
    // Limit history size
    if (data.healthHistory.size() > 1000) {
        data.healthHistory.remove(0, 500);
    }
    
    // Count as alert if health is low
    if (health < 60) {
        data.alertCount++;
    }
}

void AnalyticsDashboard::recordSubsystemHealth(const QString& componentId, const QString& subsystem, qreal health)
{
    if (m_componentData.contains(componentId)) {
        m_componentData[componentId].subsystemHealth[subsystem].append(health);
        
        // Limit subsystem history
        if (m_componentData[componentId].subsystemHealth[subsystem].size() > 500) {
            m_componentData[componentId].subsystemHealth[subsystem].remove(0, 250);
        }
    }
}

void AnalyticsDashboard::recordMessage(const QString& componentId, qint64 timestamp)
{
    if (m_componentData.contains(componentId)) {
        m_componentData[componentId].messageTimestamps.append(timestamp);
        m_componentData[componentId].totalMessages++;
        
        // Limit message history
        if (m_componentData[componentId].messageTimestamps.size() > 1000) {
            m_componentData[componentId].messageTimestamps.remove(0, 500);
        }
    }
}

void AnalyticsDashboard::addComponent(const QString& componentId, const QString& type)
{
    if (!m_componentData.contains(componentId)) {
        ComponentHealthData data;
        data.componentId = componentId;
        data.type = type.toUpper();
        data.currentHealth = 0.0;  // Initialize to 0% until health message is received
        data.currentStatus = "UNKNOWN";
        data.totalMessages = 0;
        data.alertCount = 0;
        data.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
        m_componentData[componentId] = data;
        
        m_componentTypeCount[type.toUpper()]++;
        m_componentFilterCombo->addItem(componentId);
    }
}

void AnalyticsDashboard::removeComponent(const QString& componentId)
{
    if (m_componentData.contains(componentId)) {
        QString type = m_componentData[componentId].type;
        m_componentData.remove(componentId);
        
        m_componentTypeCount[type]--;
        if (m_componentTypeCount[type] <= 0) {
            m_componentTypeCount.remove(type);
        }
        
        // Remove from filter combo
        for (int i = 0; i < m_componentFilterCombo->count(); i++) {
            if (m_componentFilterCombo->itemText(i) == componentId) {
                m_componentFilterCombo->removeItem(i);
                break;
            }
        }
    }
}

void AnalyticsDashboard::clear()
{
    m_componentData.clear();
    m_componentTypeCount.clear();
    m_componentFilterCombo->clear();
    m_componentFilterCombo->addItem("ALL COMPONENTS");
    updateAllCharts();
}

void AnalyticsDashboard::refreshDashboard()
{
    // Update all charts when refresh button is clicked
    updateAllCharts();
}

void AnalyticsDashboard::onThemeChanged()
{
    // Update theme colors
    ThemeManager& tm = ThemeManager::instance();
    
    m_bgColor = tm.windowBackground();
    m_textColor = tm.primaryText();
    m_gridColor = tm.chartGridLine();
    m_chartBgColor = tm.chartBackground();
    
    // Refresh all charts with new theme
    updateAllCharts();
}

void AnalyticsDashboard::updateAllCharts()
{
    // Update all charts in the 2x2 grid based on their current type
    for (int i = 0; i < 4; i++) {
        if (m_chartGrids[i].chartView) {
            QString componentFilter = m_componentFilterCombo->currentText();
            if (componentFilter == "ALL COMPONENTS") {
                componentFilter = "";
            }
            updateChart(m_chartGrids[i].chartView, m_chartGrids[i].currentChartType, componentFilter);
        }
    }
    
    // Also update KPIs
    updateKPIs();
}

void AnalyticsDashboard::updateHealthTrendChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    
    QChart* chart = chartView->chart();
    
    // Clear existing series
    QList<QAbstractSeries*> oldSeries = chart->series();
    for (QAbstractSeries* series : oldSeries) {
        chart->removeSeries(series);
        delete series;
    }
    
    // Clear existing axes
    QList<QAbstractAxis*> oldAxes = chart->axes();
    for (QAbstractAxis* axis : oldAxes) {
        chart->removeAxis(axis);
        delete axis;
    }
    
    chart->setTitle("Component Health Trend");
    
    // Filter components if needed
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    // Use vibrant color palette for different components
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;
    
    // Create line series for each component with enhanced styling
    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        if (it->healthHistory.isEmpty()) continue;
        
        QLineSeries* series = new QLineSeries();
        series->setName(it->componentId);
        
        for (const auto& point : it->healthHistory) {
            series->append(point.first, point.second);
        }
        
        // Enhanced line styling with vibrant colors
        QColor lineColor = palette[colorIndex % palette.size()];
        QPen pen(lineColor);
        pen.setWidth(3);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        series->setPen(pen);
        
        // Add point markers for better visibility
        series->setPointsVisible(true);
        series->setPointLabelsVisible(false);
        
        chart->addSeries(series);
        colorIndex++;
    }
    
    // Add axes with professional formatting
    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Time (relative)");
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineColor(m_gridColor);
    axisX->setMinorGridLineColor(m_gridColor.lighter(110));
    axisX->setLabelsAngle(0);
    axisX->setLabelFormat("%.0f");
    
    QFont axisFont;
    axisFont.setPointSize(7);
    axisFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisX->setLabelsFont(axisFont);
    
    QFont axisTitleFont;
    axisTitleFont.setPointSize(8);
    axisTitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisX->setTitleFont(axisTitleFont);
    
    chart->addAxis(axisX, Qt::AlignBottom);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Health (%)");
    axisY->setRange(0, 105);
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setMinorGridLineColor(m_gridColor.lighter(110));
    axisY->setLabelFormat("%.0f%%");
    axisY->setTickCount(11);
    axisY->setLabelsFont(axisFont);
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    
    // Attach axes to series
    for (QAbstractSeries* series : chart->series()) {
        series->attachAxis(axisX);
        series->attachAxis(axisY);
    }
    
    applyChartTheme(chart);
}

void AnalyticsDashboard::updateComponentDistributionChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    
    QChart* chart = chartView->chart();
    
    // Clear existing series
    QList<QAbstractSeries*> oldSeries = chart->series();
    for (QAbstractSeries* series : oldSeries) {
        chart->removeSeries(series);
        delete series;
    }
    
    chart->setTitle("Component Type Distribution");
    
    QPieSeries* pieSeries = new QPieSeries();
    
    // Use vibrant color palette for pie slices
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;
    
    qreal totalCount = 0;
    for (auto it = m_componentTypeCount.begin(); it != m_componentTypeCount.end(); ++it) {
        totalCount += it.value();
    }
    
    for (auto it = m_componentTypeCount.begin(); it != m_componentTypeCount.end(); ++it) {
        if (it.value() > 0) {
            qreal percentage = (it.value() / totalCount) * 100.0;
            QString label = QString("%1\n%2 (%3%)")
                .arg(it.key())
                .arg(it.value())
                .arg(QString::number(percentage, 'f', 1));
            
            QPieSlice* slice = pieSeries->append(label, it.value());
            
            // Use vibrant colors from palette
            QColor sliceColor = palette[colorIndex % palette.size()];
            slice->setColor(sliceColor);
            slice->setLabelVisible(true);
            slice->setLabelColor(m_textColor);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setBorderColor(m_chartBgColor);
            slice->setBorderWidth(2);
            
            // Professional label font
            QFont labelFont;
            labelFont.setPointSize(7);
            labelFont.setBold(true);
            labelFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
            slice->setLabelFont(labelFont);
            
            // Enhanced hover effect
            slice->setExplodeDistanceFactor(0.08);
            
            colorIndex++;
        }
    }
    
    pieSeries->setHoleSize(0.45);
    pieSeries->setPieSize(0.85);
    chart->addSeries(pieSeries);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    applyChartTheme(chart);
}

void AnalyticsDashboard::updateSubsystemPerformanceChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    
    QChart* chart = chartView->chart();
    
    // Clear existing series and axes
    QList<QAbstractSeries*> oldSeries = chart->series();
    for (QAbstractSeries* series : oldSeries) {
        chart->removeSeries(series);
        delete series;
    }
    QList<QAbstractAxis*> oldAxes = chart->axes();
    for (QAbstractAxis* axis : oldAxes) {
        chart->removeAxis(axis);
        delete axis;
    }
    
    chart->setTitle("Subsystem Performance");
    
    // Calculate average subsystem health
    QMap<QString, qreal> subsystemAvgHealth;
    QMap<QString, int> subsystemCount;
    
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    for (const auto& compData : dataToShow) {
        for (auto it = compData.subsystemHealth.begin(); it != compData.subsystemHealth.end(); ++it) {
            if (!it.value().isEmpty()) {
                qreal avg = 0.0;
                for (qreal val : it.value()) {
                    avg += val;
                }
                avg /= it.value().size();
                
                subsystemAvgHealth[it.key()] += avg;
                subsystemCount[it.key()]++;
            }
        }
    }
    
    QBarSeries* barSeries = new QBarSeries();
    
    // Use vibrant colors - create separate bar set for each subsystem for varied colors
    QStringList categories;
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;
    
    for (auto it = subsystemAvgHealth.begin(); it != subsystemAvgHealth.end(); ++it) {
        if (subsystemCount[it.key()] > 0) {
            qreal avgHealth = it.value() / subsystemCount[it.key()];
            
            QBarSet* set = new QBarSet(it.key());
            *set << avgHealth;
            
            // Use vibrant colors from palette
            QColor barColor = palette[colorIndex % palette.size()];
            set->setColor(barColor);
            set->setBorderColor(m_chartBgColor);
            
            barSeries->append(set);
            categories << it.key();
            colorIndex++;
        }
    }
    
    barSeries->setBarWidth(0.75);
    chart->addSeries(barSeries);
    
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-45);
    axisX->setGridLineVisible(false);
    
    QFont axisFont;
    axisFont.setPointSize(7);
    axisFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisX->setLabelsFont(axisFont);
    
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setRange(0, 105);
    axisY->setTitleText("Health (%)");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setMinorGridLineColor(m_gridColor.lighter(110));
    axisY->setLabelFormat("%.0f%%");
    axisY->setTickCount(11);
    axisY->setLabelsFont(axisFont);
    
    QFont axisTitleFont;
    axisTitleFont.setPointSize(8);
    axisTitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    
    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

void AnalyticsDashboard::updateMessageFrequencyChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    
    QChart* chart = chartView->chart();
    
    // Clear existing series and axes
    QList<QAbstractSeries*> oldSeries = chart->series();
    for (QAbstractSeries* series : oldSeries) {
        chart->removeSeries(series);
        delete series;
    }
    QList<QAbstractAxis*> oldAxes = chart->axes();
    for (QAbstractAxis* axis : oldAxes) {
        chart->removeAxis(axis);
        delete axis;
    }
    
    chart->setTitle("Message Frequency");
    
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    QBarSeries* barSeries = new QBarSeries();
    
    // Use vibrant colors - create separate bar set for each component
    QStringList categories;
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;
    
    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        QBarSet* set = new QBarSet(it.key());
        *set << it->totalMessages;
        
        // Use vibrant colors from palette
        QColor barColor = palette[colorIndex % palette.size()];
        set->setColor(barColor);
        set->setBorderColor(m_chartBgColor);
        
        barSeries->append(set);
        categories << it.key();
        colorIndex++;
    }
    
    barSeries->setBarWidth(0.75);
    chart->addSeries(barSeries);
    
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-45);
    axisX->setGridLineVisible(false);
    
    QFont axisFont;
    axisFont.setPointSize(7);
    axisFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisX->setLabelsFont(axisFont);
    
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Message Count");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setMinorGridLineColor(m_gridColor.lighter(110));
    axisY->setLabelFormat("%.0f");
    axisY->setLabelsFont(axisFont);
    
    QFont axisTitleFont;
    axisTitleFont.setPointSize(8);
    axisTitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    
    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

void AnalyticsDashboard::updateAlertHistoryChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    
    QChart* chart = chartView->chart();
    
    // Clear existing series and axes
    QList<QAbstractSeries*> oldSeries = chart->series();
    for (QAbstractSeries* series : oldSeries) {
        chart->removeSeries(series);
        delete series;
    }
    QList<QAbstractAxis*> oldAxes = chart->axes();
    for (QAbstractAxis* axis : oldAxes) {
        chart->removeAxis(axis);
        delete axis;
    }
    
    chart->setTitle("Alert History");
    
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    QBarSeries* barSeries = new QBarSeries();
    
    // Use vibrant colors - create separate bar set for each component
    QStringList categories;
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;
    
    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        QBarSet* set = new QBarSet(it.key());
        *set << it->alertCount;
        
        // Use vibrant colors from palette, with preference for warmer colors for alerts
        QColor barColor = palette[(colorIndex + 4) % palette.size()]; // Offset to get reds/oranges
        set->setColor(barColor);
        set->setBorderColor(m_chartBgColor);
        
        barSeries->append(set);
        categories << it.key();
        colorIndex++;
    }
    
    barSeries->setBarWidth(0.75);
    chart->addSeries(barSeries);
    
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-45);
    axisX->setGridLineVisible(false);
    
    QFont axisFont;
    axisFont.setPointSize(7);
    axisFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisX->setLabelsFont(axisFont);
    
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Alert Count");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setMinorGridLineColor(m_gridColor.lighter(110));
    axisY->setLabelFormat("%.0f");
    axisY->setLabelsFont(axisFont);
    
    QFont axisTitleFont;
    axisTitleFont.setPointSize(8);
    axisTitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    
    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

void AnalyticsDashboard::updateComponentComparisonChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    
    QChart* chart = chartView->chart();
    
    // Clear existing series and axes
    QList<QAbstractSeries*> oldSeries = chart->series();
    for (QAbstractSeries* series : oldSeries) {
        chart->removeSeries(series);
        delete series;
    }
    QList<QAbstractAxis*> oldAxes = chart->axes();
    for (QAbstractAxis* axis : oldAxes) {
        chart->removeAxis(axis);
        delete axis;
    }
    
    chart->setTitle("Component Comparison");
    
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    QBarSeries* barSeries = new QBarSeries();
    
    // Use vibrant colors - create separate bar set for each component
    QStringList categories;
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;
    
    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        categories << it.key();
        qreal health = it->currentHealth;
        
        QBarSet* set = new QBarSet(it.key());
        set->append(health);
        
        // Use vibrant colors from palette, with health-based color selection
        QColor barColor;
        if (health >= 90) {
            barColor = palette[1]; // Green
        } else if (health >= 75) {
            barColor = palette[0]; // Blue
        } else if (health >= 60) {
            barColor = palette[3]; // Yellow
        } else {
            barColor = palette[4]; // Red
        }
        
        set->setColor(barColor);
        set->setBorderColor(m_chartBgColor);
        
        barSeries->append(set);
        colorIndex++;
    }
    
    barSeries->setBarWidth(0.65);
    chart->addSeries(barSeries);
    
    QBarCategoryAxis* axisY = new QBarCategoryAxis();
    axisY->append(categories);
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineVisible(false);
    
    QFont axisFont;
    axisFont.setPointSize(7);
    axisFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisY->setLabelsFont(axisFont);
    
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    
    QValueAxis* axisX = new QValueAxis();
    axisX->setRange(0, 105);
    axisX->setTitleText("Health (%)");
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineColor(m_gridColor);
    axisX->setMinorGridLineColor(m_gridColor.lighter(110));
    axisX->setLabelFormat("%.0f%%");
    axisX->setTickCount(11);
    axisX->setLabelsFont(axisFont);
    
    QFont axisTitleFont;
    axisTitleFont.setPointSize(8);
    axisTitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    axisX->setTitleFont(axisTitleFont);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

void AnalyticsDashboard::applyChartTheme(QChart* chart)
{
    chart->setBackgroundBrush(QBrush(m_chartBgColor));
    chart->setTitleBrush(QBrush(m_textColor));
    chart->setBackgroundRoundness(8);
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setAnimationDuration(800);
    chart->setAnimationEasingCurve(QEasingCurve::OutCubic);
    
    // Modern title font with better styling
    QFont titleFont;
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    titleFont.setWeight(QFont::Bold);
    titleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.6);
    chart->setTitleFont(titleFont);
    
    // Enhanced legend styling
    if (chart->legend()) {
        chart->legend()->setLabelColor(m_textColor);
        chart->legend()->setAlignment(Qt::AlignBottom);
        chart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);
        chart->legend()->setShowToolTips(true);
        
        QFont legendFont;
        legendFont.setPointSize(7);
        legendFont.setBold(false);
        legendFont.setWeight(QFont::Medium);
        legendFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
        legendFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.2);
        chart->legend()->setFont(legendFont);
        
        // Enhanced legend styling with better visual appearance
        chart->legend()->setBrush(QBrush(m_chartBgColor.lighter(102)));
        chart->legend()->setPen(QPen(m_gridColor.lighter(110), 1));
        chart->legend()->setBorderColor(m_gridColor.lighter(110));
    }
    
    // Optimized margins for better data visibility
    chart->setMargins(QMargins(10, 12, 10, 10));
    
    // Drop shadow effect for depth (simulated via styling)
    chart->setDropShadowEnabled(false); // We'll use CSS shadows instead
}

void AnalyticsDashboard::enableChartTooltips(QChartView* chartView)
{
    if (!chartView) return;
    
    // Enable mouse tracking for tooltips
    chartView->setMouseTracking(true);
    chartView->setInteractive(true);
    
    // Note: Qt Charts handles tooltips automatically for series with point labels
    // We've already configured point visibility in series creation
    // For enhanced tooltips, we rely on Qt's built-in tooltip mechanism
    
    // Set a general tooltip for the chart view
    chartView->setToolTip("Hover over data points for detailed information");
}

void AnalyticsDashboard::onExportToPDF()
{
    // Open file dialog to select where to save the PDF
    QString fileName = QFileDialog::getSaveFileName(this,
                                                     "Export Dashboard to PDF",
                                                     "analytics_dashboard.pdf",
                                                     "PDF Files (*.pdf)");
    
    if (fileName.isEmpty()) {
        return; // User cancelled
    }
    
    // Ensure .pdf extension
    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        fileName += ".pdf";
    }
    
    // Create printer for PDF export
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageSize(QPrinter::A4);
    
    // Create painter
    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::warning(this, "Export Failed", "Unable to create PDF file.");
        return;
    }
    
    // Get the page rectangle
    QRect pageRect = printer.pageRect(QPrinter::DevicePixel).toRect();
    int pageWidth = pageRect.width();
    int pageHeight = pageRect.height();
    
    // Render the entire dashboard widget to the PDF
    // We'll capture the central widget as a pixmap and then render it
    QPixmap dashboardPixmap(m_centralWidget->size());
    m_centralWidget->render(&dashboardPixmap);
    
    // Scale to fit the page while maintaining aspect ratio
    QPixmap scaledPixmap = dashboardPixmap.scaled(pageWidth, pageHeight, 
                                                    Qt::KeepAspectRatio, 
                                                    Qt::SmoothTransformation);
    
    // Center the image on the page
    int x = (pageWidth - scaledPixmap.width()) / 2;
    int y = (pageHeight - scaledPixmap.height()) / 2;
    
    painter.drawPixmap(x, y, scaledPixmap);
    painter.end();
    
    QMessageBox::information(this, "Export Successful", 
                              QString("Dashboard exported successfully to:\n%1").arg(fileName));
}
