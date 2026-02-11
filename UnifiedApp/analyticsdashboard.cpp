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
    resize(1600, 1000);
    
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
    generateSampleData();
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
    AppTheme theme = tm.currentTheme();
    
    if (theme == AppTheme::Dark) {
        m_bgColor = QColor(30, 30, 30);
        m_textColor = QColor(220, 220, 220);
        m_gridColor = QColor(60, 60, 60);
        m_chartBgColor = QColor(45, 45, 45);
    } else {
        m_bgColor = QColor(245, 245, 245);
        m_textColor = QColor(40, 40, 40);
        m_gridColor = QColor(200, 200, 200);
        m_chartBgColor = QColor(255, 255, 255);
    }
    
    // Create scroll area for dashboard
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("dashboardScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    
    m_centralWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(25, 25, 25, 25);
    
    // ========== HEADER SECTION ==========
    QWidget* headerWidget = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setSpacing(15);
    
    QLabel* titleLabel = new QLabel("DATA ANALYTICS DASHBOARD");
    titleLabel->setObjectName("dashboardTitle");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(26);
    titleFont.setBold(true);
    titleFont.setFamily("Arial");
    titleLabel->setFont(titleFont);
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    // Controls
    QLabel* filterLabel = new QLabel("COMPONENT:");
    filterLabel->setObjectName("filterLabel");
    QFont labelFont = filterLabel->font();
    labelFont.setPointSize(11);
    labelFont.setBold(true);
    filterLabel->setFont(labelFont);
    
    m_componentFilterCombo = new QComboBox();
    m_componentFilterCombo->setObjectName("dashboardCombo");
    m_componentFilterCombo->addItem("ALL COMPONENTS");
    m_componentFilterCombo->setFixedWidth(200);
    QFont comboFont;
    comboFont.setPointSize(11);
    m_componentFilterCombo->setFont(comboFont);
    connect(m_componentFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AnalyticsDashboard::onComponentFilterChanged);
    
    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->setObjectName("dashboardCombo");
    m_timeRangeCombo->addItems({"LAST HOUR", "LAST 6 HOURS", "LAST 24 HOURS", "LAST WEEK"});
    m_timeRangeCombo->setCurrentIndex(2);
    m_timeRangeCombo->setFixedWidth(150);
    m_timeRangeCombo->setFont(comboFont);
    
    m_refreshBtn = new QPushButton("REFRESH");
    m_refreshBtn->setObjectName("dashboardRefreshBtn");
    m_refreshBtn->setFixedWidth(110);
    m_refreshBtn->setFixedHeight(32);
    QFont btnFont;
    btnFont.setPointSize(11);
    btnFont.setBold(true);
    m_refreshBtn->setFont(btnFont);
    connect(m_refreshBtn, &QPushButton::clicked, this, &AnalyticsDashboard::refreshDashboard);
    
    m_exportBtn = new QPushButton("EXPORT PDF");
    m_exportBtn->setObjectName("dashboardExportBtn");
    m_exportBtn->setFixedWidth(120);
    m_exportBtn->setFixedHeight(32);
    m_exportBtn->setFont(btnFont);
    connect(m_exportBtn, &QPushButton::clicked, this, &AnalyticsDashboard::onExportToPDF);
    
    headerLayout->addWidget(filterLabel);
    headerLayout->addWidget(m_componentFilterCombo);
    headerLayout->addWidget(m_timeRangeCombo);
    headerLayout->addWidget(m_refreshBtn);
    headerLayout->addWidget(m_exportBtn);
    
    m_mainLayout->addWidget(headerWidget);
    
    // ========== KPI SECTION ==========
    m_mainLayout->addWidget(createKPISection());
    
    // Add a subtle divider
    QFrame* divider = new QFrame();
    divider->setObjectName("dashboardDivider");
    divider->setFrameShape(QFrame::HLine);
    divider->setFixedHeight(2);
    m_mainLayout->addWidget(divider);
    
    // ========== 2x2 CHARTS GRID ==========
    QGridLayout* chartsGrid = new QGridLayout();
    chartsGrid->setObjectName("chartsGrid");
    chartsGrid->setSpacing(18);
    chartsGrid->setContentsMargins(0, 0, 0, 0);
    
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
    QWidget* kpiWidget = new QWidget();
    kpiWidget->setObjectName("kpiSection");
    QHBoxLayout* kpiLayout = new QHBoxLayout(kpiWidget);
    kpiLayout->setSpacing(15);
    kpiLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create KPI cards with military-grade design
    QWidget* card1 = createKPICard("TOTAL COMPONENTS", "0", "MONITORED", QColor(52, 152, 219));
    QWidget* card2 = createKPICard("ACTIVE", "0", "ONLINE", QColor(46, 204, 113));
    QWidget* card3 = createKPICard("AVG HEALTH", "0%", "SYSTEM-WIDE", QColor(241, 196, 15));
    QWidget* card4 = createKPICard("ALERTS", "0", "TOTAL COUNT", QColor(231, 76, 60));
    
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
    card->setMinimumHeight(130);
    card->setMaximumHeight(150);
    // Border color styling moved to QSS
    card->setStyleSheet(QString(
        "QWidget#kpiCard { border-left: 4px solid %1; }"
    ).arg(color.name()));
    
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(8);
    cardLayout->setContentsMargins(18, 18, 18, 18);
    
    QLabel* titleLabel = new QLabel(title);
    titleLabel->setObjectName("kpiTitle");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    titleFont.setFamily("Arial");
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QString("color: %1;").arg(color.name()));
    titleLabel->setAlignment(Qt::AlignLeft);
    
    QLabel* valueLabel = new QLabel(value);
    valueLabel->setObjectName("kpiValue");
    QFont valueFont = valueLabel->font();
    valueFont.setPointSize(38);
    valueFont.setBold(true);
    valueFont.setFamily("Arial");
    valueLabel->setFont(valueFont);
    valueLabel->setAlignment(Qt::AlignCenter);
    
    // Store label for updates
    if (title == "TOTAL COMPONENTS") m_totalComponentsLabel = valueLabel;
    else if (title == "ACTIVE") m_activeComponentsLabel = valueLabel;
    else if (title == "AVG HEALTH") m_avgHealthLabel = valueLabel;
    else if (title == "ALERTS") m_totalAlertsLabel = valueLabel;
    
    QLabel* subtitleLabel = new QLabel(subtitle);
    subtitleLabel->setObjectName("kpiSubtitle");
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(11);
    subtitleFont.setBold(true);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(valueLabel);
    cardLayout->addWidget(subtitleLabel);
    
    return card;
}

QWidget* AnalyticsDashboard::createChartGrid(int gridIndex, ChartType initialType)
{
    QWidget* container = new QWidget();
    container->setObjectName("chartContainer");
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setSpacing(10);
    layout->setContentsMargins(5, 5, 5, 5);
    
    // Create dropdown for chart type selection
    QComboBox* chartTypeCombo = new QComboBox();
    chartTypeCombo->setObjectName("chartTypeCombo");
    QFont comboFont;
    comboFont.setPointSize(11);
    comboFont.setBold(true);
    chartTypeCombo->setFont(comboFont);
    chartTypeCombo->addItem("HEALTH TREND", static_cast<int>(ChartType::HealthTrend));
    chartTypeCombo->addItem("COMPONENT DISTRIBUTION", static_cast<int>(ChartType::ComponentDistribution));
    chartTypeCombo->addItem("SUBSYSTEM PERFORMANCE", static_cast<int>(ChartType::SubsystemPerformance));
    chartTypeCombo->addItem("MESSAGE FREQUENCY", static_cast<int>(ChartType::MessageFrequency));
    chartTypeCombo->addItem("ALERT HISTORY", static_cast<int>(ChartType::AlertHistory));
    chartTypeCombo->addItem("COMPONENT COMPARISON", static_cast<int>(ChartType::ComponentComparison));
    
    // Set initial selection
    chartTypeCombo->setCurrentIndex(static_cast<int>(initialType));
    
    // Connect dropdown to update handler
    connect(chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, gridIndex](int index) {
                this->onChartTypeChanged(gridIndex);
            });
    
    // Create chart view
    QChart* chart = new QChart();
    applyChartTheme(chart);
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(400);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Enable tooltips
    enableChartTooltips(chartView);
    
    layout->addWidget(chartTypeCombo);
    layout->addWidget(chartView);
    
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
    
    // Update KPI labels
    if (m_totalComponentsLabel) m_totalComponentsLabel->setText(QString::number(totalComponents));
    if (m_activeComponentsLabel) m_activeComponentsLabel->setText(QString::number(activeComponents));
    if (m_avgHealthLabel) m_avgHealthLabel->setText(QString("%1%").arg(qRound(avgHealth)));
    if (m_totalAlertsLabel) m_totalAlertsLabel->setText(QString::number(totalAlerts));
}

QColor AnalyticsDashboard::getHealthColor(qreal health)
{
    if (health >= 90) return QColor(46, 204, 113);  // Green
    if (health >= 75) return QColor(52, 152, 219);  // Blue
    if (health >= 60) return QColor(241, 196, 15);  // Yellow
    if (health >= 40) return QColor(230, 126, 34);  // Orange
    return QColor(231, 76, 60);  // Red
}

QString AnalyticsDashboard::getHealthStatus(qreal health)
{
    if (health >= 90) return "EXCELLENT";
    if (health >= 75) return "GOOD";
    if (health >= 60) return "FAIR";
    if (health >= 40) return "POOR";
    return "CRITICAL";
}

void AnalyticsDashboard::recordComponentHealth(const QString& componentId, const QString& color, qreal health, qint64 timestamp)
{
    if (!m_componentData.contains(componentId)) {
        ComponentHealthData data;
        data.componentId = componentId;
        data.type = "UNKNOWN";
        data.currentHealth = health;
        data.currentStatus = getHealthStatus(health);
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
        data.currentHealth = 100.0;
        data.currentStatus = "EXCELLENT";
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
    AppTheme theme = tm.currentTheme();
    
    if (theme == AppTheme::Dark) {
        m_bgColor = QColor(30, 30, 30);
        m_textColor = QColor(220, 220, 220);
        m_gridColor = QColor(60, 60, 60);
        m_chartBgColor = QColor(45, 45, 45);
    } else {
        m_bgColor = QColor(245, 245, 245);
        m_textColor = QColor(40, 40, 40);
        m_gridColor = QColor(200, 200, 200);
        m_chartBgColor = QColor(255, 255, 255);
    }
    
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
    
    chart->setTitle("COMPONENT HEALTH TREND");
    
    // Filter components if needed
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    // Create line series for each component
    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        if (it->healthHistory.isEmpty()) continue;
        
        QLineSeries* series = new QLineSeries();
        series->setName(it->componentId);
        
        for (const auto& point : it->healthHistory) {
            series->append(point.first, point.second);
        }
        
        chart->addSeries(series);
    }
    
    // Add axes
    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Time");
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineColor(m_gridColor);
    chart->addAxis(axisX, Qt::AlignBottom);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Health %");
    axisY->setRange(0, 100);
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
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
    
    chart->setTitle("COMPONENT TYPE DISTRIBUTION");
    
    QPieSeries* pieSeries = new QPieSeries();
    
    QVector<QColor> pieColors = {
        QColor(52, 152, 219),
        QColor(46, 204, 113),
        QColor(241, 196, 15),
        QColor(231, 76, 60),
        QColor(155, 89, 182)
    };
    
    int colorIndex = 0;
    for (auto it = m_componentTypeCount.begin(); it != m_componentTypeCount.end(); ++it) {
        if (it.value() > 0) {
            QPieSlice* slice = pieSeries->append(it.key(), it.value());
            slice->setColor(pieColors[colorIndex % pieColors.size()]);
            slice->setLabelVisible(true);
            slice->setLabelColor(m_textColor);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            colorIndex++;
        }
    }
    
    pieSeries->setHoleSize(0.4);
    pieSeries->setPieSize(0.75);
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
    
    chart->setTitle("SUBSYSTEM PERFORMANCE");
    
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
    QBarSet* set = new QBarSet("Avg Health");
    set->setColor(QColor(52, 152, 219));
    
    QStringList categories;
    for (auto it = subsystemAvgHealth.begin(); it != subsystemAvgHealth.end(); ++it) {
        if (subsystemCount[it.key()] > 0) {
            qreal avgHealth = it.value() / subsystemCount[it.key()];
            *set << avgHealth;
            categories << it.key();
        }
    }
    
    barSeries->append(set);
    chart->addSeries(barSeries);
    
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-45);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setTitleText("Health %");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f");
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
    
    chart->setTitle("MESSAGE FREQUENCY");
    
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    QBarSeries* barSeries = new QBarSeries();
    QBarSet* set = new QBarSet("Messages");
    set->setColor(QColor(52, 152, 219));
    
    QStringList categories;
    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        categories << it.key();
        *set << it->totalMessages;
    }
    
    barSeries->append(set);
    chart->addSeries(barSeries);
    
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-45);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Messages");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f");
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
    
    chart->setTitle("ALERT HISTORY");
    
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    QBarSeries* barSeries = new QBarSeries();
    QBarSet* set = new QBarSet("Alerts");
    set->setColor(QColor(231, 76, 60));
    
    QStringList categories;
    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        categories << it.key();
        *set << it->alertCount;
    }
    
    barSeries->append(set);
    chart->addSeries(barSeries);
    
    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-45);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Alerts");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f");
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
    
    chart->setTitle("COMPONENT COMPARISON");
    
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) {
        dataToShow = m_componentData;
    } else if (m_componentData.contains(componentFilter)) {
        dataToShow[componentFilter] = m_componentData[componentFilter];
    }
    
    QBarSeries* barSeries = new QBarSeries();
    QBarSet* healthSet = new QBarSet("Health");
    
    QStringList categories;
    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        categories << it.key();
        qreal health = it->currentHealth;
        healthSet->append(health);
    }
    
    healthSet->setColor(QColor(46, 204, 113));
    barSeries->append(healthSet);
    chart->addSeries(barSeries);
    
    QBarCategoryAxis* axisY = new QBarCategoryAxis();
    axisY->append(categories);
    axisY->setLabelsColor(m_textColor);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    
    QValueAxis* axisX = new QValueAxis();
    axisX->setRange(0, 100);
    axisX->setTitleText("Health %");
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineColor(m_gridColor);
    axisX->setLabelFormat("%.0f");
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

void AnalyticsDashboard::applyChartTheme(QChart* chart)
{
    chart->setBackgroundBrush(QBrush(m_chartBgColor));
    chart->setTitleBrush(QBrush(m_textColor));
    chart->setBackgroundRoundness(4);
    
    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleFont.setFamily("Arial");
    chart->setTitleFont(titleFont);
    
    if (chart->legend()) {
        chart->legend()->setLabelColor(m_textColor);
        QFont legendFont;
        legendFont.setPointSize(12);
        legendFont.setBold(false);
        chart->legend()->setFont(legendFont);
    }
    
    // Increased margins to prevent text chopping
    chart->setMargins(QMargins(20, 20, 20, 20));
}

void AnalyticsDashboard::enableChartTooltips(QChartView* chartView)
{
    if (!chartView) return;
    
    // Enable mouse tracking for tooltips
    chartView->setMouseTracking(true);
    
    // Tooltips will be configured per series when we create them
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
