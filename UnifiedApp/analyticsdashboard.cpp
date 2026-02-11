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

AnalyticsDashboard::AnalyticsDashboard(QWidget* parent)
    : QMainWindow(parent)
    , m_scrollArea(nullptr)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_healthTrendChart(nullptr)
    , m_componentDistChart(nullptr)
    , m_subsystemPerfChart(nullptr)
    , m_healthAreaChart(nullptr)
    , m_messageFreqChart(nullptr)
    , m_telemetryChart(nullptr)
    , m_alertHistoryChart(nullptr)
    , m_comparisonChart(nullptr)
    , m_efficiencyChart(nullptr)
    , m_upTimeChart(nullptr)
    , m_totalComponentsLabel(nullptr)
    , m_activeComponentsLabel(nullptr)
    , m_avgHealthLabel(nullptr)
    , m_totalAlertsLabel(nullptr)
    , m_systemEfficiencyLabel(nullptr)
    , m_upTimeLabel(nullptr)
    , m_timeRangeCombo(nullptr)
    , m_componentFilterCombo(nullptr)
    , m_refreshBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_updateTimer(nullptr)
{
    setWindowTitle("Advanced Analytics Dashboard");
    resize(1600, 1000);
    
    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &AnalyticsDashboard::onThemeChanged);
    
    setupUI();
    generateSampleData();
    updateCharts();
    
    // Setup auto-refresh timer
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &AnalyticsDashboard::updateCharts);
    m_updateTimer->start(5000); // Update every 5 seconds
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
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    
    m_centralWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // ========== HEADER SECTION ==========
    QWidget* headerWidget = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    
    QLabel* titleLabel = new QLabel("Data Analytics Dashboard");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QString("color: %1;").arg(m_textColor.name()));
    
    QLabel* subtitleLabel = new QLabel("Real-time Component Health & Performance Monitoring");
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(12);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet(QString("color: %1;").arg(m_textColor.lighter(130).name()));
    
    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);
    
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    
    // Controls
    m_timeRangeCombo = new QComboBox();
    m_timeRangeCombo->addItems({"Last Hour", "Last 6 Hours", "Last 24 Hours", "Last Week", "Last Month"});
    m_timeRangeCombo->setCurrentIndex(2);
    m_timeRangeCombo->setMinimumWidth(150);
    
    m_componentFilterCombo = new QComboBox();
    m_componentFilterCombo->addItem("All Components");
    m_componentFilterCombo->setMinimumWidth(150);
    
    m_refreshBtn = new QPushButton("Refresh");
    m_refreshBtn->setMinimumWidth(100);
    connect(m_refreshBtn, &QPushButton::clicked, this, &AnalyticsDashboard::refreshDashboard);
    
    m_exportBtn = new QPushButton("Export Data");
    m_exportBtn->setMinimumWidth(100);
    
    headerLayout->addWidget(new QLabel("Time Range:"));
    headerLayout->addWidget(m_timeRangeCombo);
    headerLayout->addWidget(new QLabel("Filter:"));
    headerLayout->addWidget(m_componentFilterCombo);
    headerLayout->addWidget(m_refreshBtn);
    headerLayout->addWidget(m_exportBtn);
    
    m_mainLayout->addWidget(headerWidget);
    
    // ========== KPI SECTION ==========
    m_mainLayout->addWidget(createKPISection());
    
    // ========== CHARTS SECTION ==========
    createCharts();
    
    // Row 1: Health Trend and Component Distribution
    QWidget* row1Widget = new QWidget();
    QHBoxLayout* row1Layout = new QHBoxLayout(row1Widget);
    row1Layout->setSpacing(15);
    row1Layout->addWidget(m_healthTrendChart, 2);
    row1Layout->addWidget(m_componentDistChart, 1);
    m_mainLayout->addWidget(row1Widget);
    
    // Row 2: Subsystem Performance and Health Area
    QWidget* row2Widget = new QWidget();
    QHBoxLayout* row2Layout = new QHBoxLayout(row2Widget);
    row2Layout->setSpacing(15);
    row2Layout->addWidget(m_subsystemPerfChart, 1);
    row2Layout->addWidget(m_healthAreaChart, 2);
    m_mainLayout->addWidget(row2Widget);
    
    // Row 3: Message Frequency and Telemetry
    QWidget* row3Widget = new QWidget();
    QHBoxLayout* row3Layout = new QHBoxLayout(row3Widget);
    row3Layout->setSpacing(15);
    row3Layout->addWidget(m_messageFreqChart, 1);
    row3Layout->addWidget(m_telemetryChart, 2);
    m_mainLayout->addWidget(row3Widget);
    
    // Row 4: Alert History and Component Comparison
    QWidget* row4Widget = new QWidget();
    QHBoxLayout* row4Layout = new QHBoxLayout(row4Widget);
    row4Layout->setSpacing(15);
    row4Layout->addWidget(m_alertHistoryChart, 1);
    row4Layout->addWidget(m_comparisonChart, 1);
    m_mainLayout->addWidget(row4Widget);
    
    // Row 5: System Efficiency and Uptime
    QWidget* row5Widget = new QWidget();
    QHBoxLayout* row5Layout = new QHBoxLayout(row5Widget);
    row5Layout->setSpacing(15);
    row5Layout->addWidget(m_efficiencyChart, 1);
    row5Layout->addWidget(m_upTimeChart, 1);
    m_mainLayout->addWidget(row5Widget);
    
    m_scrollArea->setWidget(m_centralWidget);
    setCentralWidget(m_scrollArea);
}

QWidget* AnalyticsDashboard::createKPISection()
{
    QWidget* kpiWidget = new QWidget();
    QHBoxLayout* kpiLayout = new QHBoxLayout(kpiWidget);
    kpiLayout->setSpacing(15);
    
    // Create KPI cards
    QWidget* card1 = createKPICard("Total Components", "0", "Monitored Systems", QColor(52, 152, 219));
    QWidget* card2 = createKPICard("Active Components", "0", "Currently Online", QColor(46, 204, 113));
    QWidget* card3 = createKPICard("Average Health", "0%", "System-Wide", QColor(241, 196, 15));
    QWidget* card4 = createKPICard("Total Alerts", "0", "Last 24 Hours", QColor(231, 76, 60));
    QWidget* card5 = createKPICard("System Efficiency", "0%", "Performance Index", QColor(155, 89, 182));
    QWidget* card6 = createKPICard("Uptime", "0%", "Availability", QColor(26, 188, 156));
    
    kpiLayout->addWidget(card1);
    kpiLayout->addWidget(card2);
    kpiLayout->addWidget(card3);
    kpiLayout->addWidget(card4);
    kpiLayout->addWidget(card5);
    kpiLayout->addWidget(card6);
    
    return kpiWidget;
}

QWidget* AnalyticsDashboard::createKPICard(const QString& title, const QString& value, const QString& subtitle, const QColor& color)
{
    QWidget* card = new QWidget();
    card->setMinimumHeight(120);
    card->setStyleSheet(QString(
        "QWidget { background-color: %1; border-radius: 10px; padding: 15px; }"
    ).arg(m_chartBgColor.name()));
    
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    
    QLabel* titleLabel = new QLabel(title);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(10);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QString("color: %1;").arg(color.name()));
    
    QLabel* valueLabel = new QLabel(value);
    QFont valueFont = valueLabel->font();
    valueFont.setPointSize(28);
    valueFont.setBold(true);
    valueLabel->setFont(valueFont);
    valueLabel->setStyleSheet(QString("color: %1;").arg(m_textColor.name()));
    valueLabel->setAlignment(Qt::AlignCenter);
    
    // Store label for updates
    if (title == "Total Components") m_totalComponentsLabel = valueLabel;
    else if (title == "Active Components") m_activeComponentsLabel = valueLabel;
    else if (title == "Average Health") m_avgHealthLabel = valueLabel;
    else if (title == "Total Alerts") m_totalAlertsLabel = valueLabel;
    else if (title == "System Efficiency") m_systemEfficiencyLabel = valueLabel;
    else if (title == "Uptime") m_upTimeLabel = valueLabel;
    
    QLabel* subtitleLabel = new QLabel(subtitle);
    QFont subtitleFont = subtitleLabel->font();
    subtitleFont.setPointSize(9);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setStyleSheet(QString("color: %1;").arg(m_textColor.lighter(150).name()));
    subtitleLabel->setAlignment(Qt::AlignCenter);
    
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(valueLabel);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addStretch();
    
    return card;
}

void AnalyticsDashboard::createCharts()
{
    m_healthTrendChart = createHealthTrendChart();
    m_componentDistChart = createComponentDistributionChart();
    m_subsystemPerfChart = createSubsystemPerformanceChart();
    m_healthAreaChart = createHealthAreaChart();
    m_messageFreqChart = createMessageFrequencyChart();
    m_telemetryChart = createTelemetryChart();
    m_alertHistoryChart = createAlertHistoryChart();
    m_comparisonChart = createComponentComparisonChart();
    m_efficiencyChart = createSystemEfficiencyChart();
    m_upTimeChart = createUpTimeChart();
}

QChartView* AnalyticsDashboard::createHealthTrendChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Component Health Trend (Real-Time)");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    // Will be populated with actual data in updateCharts()
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createComponentDistributionChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Component Distribution");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createSubsystemPerformanceChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Subsystem Performance Comparison");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createHealthAreaChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Health Distribution Over Time");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createMessageFrequencyChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Message Frequency Analysis");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createTelemetryChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Telemetry Data Streams");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createAlertHistoryChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Alert History (Last 24 Hours)");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createComponentComparisonChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Component Performance Comparison");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createSystemEfficiencyChart()
{
    QChart* chart = new QChart();
    chart->setTitle("System Efficiency Metrics");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createUpTimeChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Component Uptime Analysis");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);
    
    return chartView;
}

void AnalyticsDashboard::generateSampleData()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 hourMs = 3600000; // 1 hour in milliseconds
    
    // Generate sample components
    QStringList componentTypes = {"Radar", "Antenna", "PowerSystem", "Communication", "CoolingUnit", "Computer"};
    QStringList subsystems = {"Transmitter", "Receiver", "Processor", "Monitor", "Controller"};
    
    QRandomGenerator* rng = QRandomGenerator::global();
    
    // Create 8-12 sample components
    int numComponents = 8 + rng->bounded(5);
    for (int i = 0; i < numComponents; i++) {
        QString componentId = QString("COMP_%1").arg(i + 1, 3, 10, QChar('0'));
        QString type = componentTypes[rng->bounded(componentTypes.size())];
        
        ComponentHealthData data;
        data.componentId = componentId;
        data.type = type;
        data.totalMessages = 0;
        data.alertCount = 0;
        data.lastUpdateTime = currentTime;
        
        // Generate health history (last 24 hours, every 5 minutes)
        int numPoints = 288; // 24 hours * 12 points per hour
        qreal baseHealth = 70.0 + rng->bounded(25);
        
        for (int j = 0; j < numPoints; j++) {
            qint64 timestamp = currentTime - (hourMs * 24) + (j * 5 * 60000);
            
            // Add some variation
            qreal healthVariation = (rng->bounded(1000) / 1000.0 - 0.5) * 10;
            qreal health = qBound(50.0, baseHealth + healthVariation, 100.0);
            
            // Occasionally drop health to simulate issues
            if (rng->bounded(100) < 5) {
                health -= 20 + rng->bounded(20);
                data.alertCount++;
            }
            
            data.healthHistory.append(qMakePair(timestamp, health));
            
            // Add subsystem health data
            for (const QString& subsys : subsystems) {
                qreal subsysHealth = health + (rng->bounded(1000) / 1000.0 - 0.5) * 15;
                subsysHealth = qBound(40.0, subsysHealth, 100.0);
                
                if (!data.subsystemHealth.contains(subsys)) {
                    data.subsystemHealth[subsys] = QVector<qreal>();
                }
                data.subsystemHealth[subsys].append(subsysHealth);
            }
            
            // Generate message timestamps
            if (rng->bounded(100) < 30) { // 30% chance of message
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

void AnalyticsDashboard::updateCharts()
{
    if (m_componentData.isEmpty()) {
        return;
    }
    
    // Update KPI values
    int totalComponents = m_componentData.size();
    int activeComponents = 0;
    qreal totalHealth = 0.0;
    int totalAlerts = 0;
    
    for (const auto& data : m_componentData) {
        if (data.currentHealth > 50.0) {
            activeComponents++;
        }
        totalHealth += data.currentHealth;
        totalAlerts += data.alertCount;
    }
    
    qreal avgHealth = totalComponents > 0 ? totalHealth / totalComponents : 0.0;
    qreal systemEfficiency = avgHealth * 0.95; // Simplified calculation
    qreal uptime = activeComponents * 100.0 / totalComponents;
    
    m_totalComponentsLabel->setText(QString::number(totalComponents));
    m_activeComponentsLabel->setText(QString::number(activeComponents));
    m_avgHealthLabel->setText(QString("%1%").arg(qRound(avgHealth)));
    m_totalAlertsLabel->setText(QString::number(totalAlerts));
    m_systemEfficiencyLabel->setText(QString("%1%").arg(qRound(systemEfficiency)));
    m_upTimeLabel->setText(QString("%1%").arg(qRound(uptime)));
    
    // Update Health Trend Chart (Line Chart)
    {
        QChart* chart = m_healthTrendChart->chart();
        chart->removeAllSeries();
        
        QVector<QColor> seriesColors = {
            QColor(52, 152, 219),  // Blue
            QColor(46, 204, 113),  // Green
            QColor(241, 196, 15),  // Yellow
            QColor(231, 76, 60),   // Red
            QColor(155, 89, 182),  // Purple
            QColor(26, 188, 156)   // Teal
        };
        
        int colorIndex = 0;
        int maxSeries = 6; // Limit to 6 components for readability
        int seriesCount = 0;
        
        qint64 minTime = LLONG_MAX;
        qint64 maxTime = LLONG_MIN;
        qreal minHealth = 100.0;
        qreal maxHealth = 0.0;
        
        for (auto it = m_componentData.begin(); it != m_componentData.end() && seriesCount < maxSeries; ++it) {
            QSplineSeries* series = new QSplineSeries();
            series->setName(it.key());
            series->setColor(seriesColors[colorIndex % seriesColors.size()]);
            
            // Take every 10th point for performance (show last 24 hours)
            for (int i = 0; i < it->healthHistory.size(); i += 10) {
                qint64 timestamp = it->healthHistory[i].first;
                qreal health = it->healthHistory[i].second;
                
                series->append(timestamp, health);
                
                minTime = qMin(minTime, timestamp);
                maxTime = qMax(maxTime, timestamp);
                minHealth = qMin(minHealth, health);
                maxHealth = qMax(maxHealth, health);
            }
            
            chart->addSeries(series);
            colorIndex++;
            seriesCount++;
        }
        
        // Configure axes
        QValueAxis* axisX = new QValueAxis();
        axisX->setTitleText("Time");
        axisX->setRange(minTime, maxTime);
        axisX->setLabelFormat("%i");
        axisX->setLabelsColor(m_textColor);
        axisX->setGridLineColor(m_gridColor);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("Health (%)");
        axisY->setRange(qMax(0.0, minHealth - 10), qMin(100.0, maxHealth + 10));
        axisY->setLabelFormat("%i");
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        
        for (QAbstractSeries* series : chart->series()) {
            chart->setAxisX(axisX, series);
            chart->setAxisY(axisY, series);
        }
        
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
    }
    
    // Update Component Distribution Chart (Pie Chart)
    {
        QChart* chart = m_componentDistChart->chart();
        chart->removeAllSeries();
        
        QPieSeries* series = new QPieSeries();
        
        QVector<QColor> pieColors = {
            QColor(52, 152, 219),
            QColor(46, 204, 113),
            QColor(241, 196, 15),
            QColor(231, 76, 60),
            QColor(155, 89, 182),
            QColor(26, 188, 156)
        };
        
        int colorIndex = 0;
        for (auto it = m_componentTypeCount.begin(); it != m_componentTypeCount.end(); ++it) {
            QPieSlice* slice = series->append(it.key(), it.value());
            slice->setColor(pieColors[colorIndex % pieColors.size()]);
            slice->setLabelVisible(true);
            slice->setLabelColor(m_textColor);
            colorIndex++;
        }
        
        series->setHoleSize(0.35); // Donut chart
        chart->addSeries(series);
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignRight);
    }
    
    // Update Subsystem Performance Chart (Bar Chart)
    {
        QChart* chart = m_subsystemPerfChart->chart();
        chart->removeAllSeries();
        
        QBarSeries* series = new QBarSeries();
        
        // Average subsystem health across all components
        QMap<QString, qreal> subsystemAvgHealth;
        QMap<QString, int> subsystemCount;
        
        for (const auto& compData : m_componentData) {
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
        
        QBarSet* set = new QBarSet("Average Health");
        set->setColor(QColor(52, 152, 219));
        
        QStringList categories;
        for (auto it = subsystemAvgHealth.begin(); it != subsystemAvgHealth.end(); ++it) {
            qreal avgHealth = it.value() / subsystemCount[it.key()];
            *set << avgHealth;
            categories << it.key();
        }
        
        series->append(set);
        chart->addSeries(series);
        
        QBarCategoryAxis* axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setLabelsColor(m_textColor);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setRange(0, 100);
        axisY->setTitleText("Health (%)");
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        
        chart->legend()->setVisible(false);
    }
    
    // Update Health Area Chart (Area Chart)
    {
        QChart* chart = m_healthAreaChart->chart();
        chart->removeAllSeries();
        
        // Show health zones (Critical, Warning, Good) over time
        QLineSeries* upperSeries = new QLineSeries();
        QLineSeries* lowerSeriesCritical = new QLineSeries();
        QLineSeries* lowerSeriesWarning = new QLineSeries();
        QLineSeries* lowerSeriesGood = new QLineSeries();
        
        // Get time range from first component
        if (!m_componentData.isEmpty()) {
            const auto& firstComp = m_componentData.first();
            
            // Count components in each health zone over time
            for (int i = 0; i < firstComp.healthHistory.size(); i += 20) {
                qint64 timestamp = firstComp.healthHistory[i].first;
                
                int criticalCount = 0;
                int warningCount = 0;
                int goodCount = 0;
                
                for (const auto& compData : m_componentData) {
                    if (i < compData.healthHistory.size()) {
                        qreal health = compData.healthHistory[i].second;
                        if (health < 60) criticalCount++;
                        else if (health < 80) warningCount++;
                        else goodCount++;
                    }
                }
                
                lowerSeriesCritical->append(timestamp, 0);
                upperSeries->append(timestamp, criticalCount);
                
                lowerSeriesWarning->append(timestamp, criticalCount);
                QLineSeries* tempSeries = new QLineSeries();
                tempSeries->append(timestamp, criticalCount + warningCount);
                
                lowerSeriesGood->append(timestamp, criticalCount + warningCount);
                QLineSeries* tempSeries2 = new QLineSeries();
                tempSeries2->append(timestamp, criticalCount + warningCount + goodCount);
            }
            
            // Critical zone (red)
            QAreaSeries* criticalArea = new QAreaSeries(upperSeries, lowerSeriesCritical);
            criticalArea->setName("Critical (<60%)");
            criticalArea->setColor(QColor(231, 76, 60, 100));
            criticalArea->setBorderColor(QColor(231, 76, 60));
            chart->addSeries(criticalArea);
            
            // Note: For simplicity, showing stacked distribution
            // In production, would need proper stacked area implementation
        }
        
        QValueAxis* axisX = new QValueAxis();
        axisX->setTitleText("Time");
        axisX->setLabelsColor(m_textColor);
        axisX->setGridLineColor(m_gridColor);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("Component Count");
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        
        for (QAbstractSeries* s : chart->series()) {
            chart->setAxisX(axisX, s);
            chart->setAxisY(axisY, s);
        }
        
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
    }
    
    // Update Message Frequency Chart (Scatter Plot)
    {
        QChart* chart = m_messageFreqChart->chart();
        chart->removeAllSeries();
        
        QScatterSeries* series = new QScatterSeries();
        series->setName("Message Events");
        series->setMarkerSize(8.0);
        series->setColor(QColor(52, 152, 219));
        
        // Plot message frequency over last 6 hours
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 sixHoursAgo = currentTime - (6 * 3600000);
        
        int compIndex = 0;
        for (const auto& compData : m_componentData) {
            for (qint64 msgTime : compData.messageTimestamps) {
                if (msgTime >= sixHoursAgo) {
                    series->append(msgTime, compIndex);
                }
            }
            compIndex++;
            if (compIndex >= 10) break; // Limit for readability
        }
        
        chart->addSeries(series);
        
        QValueAxis* axisX = new QValueAxis();
        axisX->setTitleText("Time");
        axisX->setRange(sixHoursAgo, currentTime);
        axisX->setLabelsColor(m_textColor);
        axisX->setGridLineColor(m_gridColor);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("Component Index");
        axisY->setRange(-1, qMin(10, m_componentData.size()));
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        
        chart->setAxisX(axisX, series);
        chart->setAxisY(axisY, series);
        
        chart->legend()->setVisible(false);
    }
    
    // Update Telemetry Chart (Multi-line)
    {
        QChart* chart = m_telemetryChart->chart();
        chart->removeAllSeries();
        
        QVector<QColor> colors = {
            QColor(52, 152, 219),
            QColor(46, 204, 113),
            QColor(241, 196, 15)
        };
        
        QStringList telemetryTypes = {"Temperature", "Power", "Signal Strength"};
        
        QRandomGenerator* rng = QRandomGenerator::global();
        
        for (int i = 0; i < telemetryTypes.size(); i++) {
            QLineSeries* series = new QLineSeries();
            series->setName(telemetryTypes[i]);
            series->setColor(colors[i]);
            
            qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
            qreal baseValue = 50.0 + rng->bounded(30);
            
            for (int j = 0; j < 100; j++) {
                qint64 timestamp = currentTime - (100 - j) * 60000; // Last 100 minutes
                qreal value = baseValue + (rng->bounded(1000) / 1000.0 - 0.5) * 20;
                series->append(timestamp, value);
            }
            
            chart->addSeries(series);
        }
        
        QValueAxis* axisX = new QValueAxis();
        axisX->setTitleText("Time");
        axisX->setLabelsColor(m_textColor);
        axisX->setGridLineColor(m_gridColor);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("Value");
        axisY->setRange(0, 100);
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        
        for (QAbstractSeries* s : chart->series()) {
            chart->setAxisX(axisX, s);
            chart->setAxisY(axisY, s);
        }
        
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
    }
    
    // Update Alert History Chart (Bar Chart)
    {
        QChart* chart = m_alertHistoryChart->chart();
        chart->removeAllSeries();
        
        QBarSeries* series = new QBarSeries();
        
        QBarSet* criticalSet = new QBarSet("Critical");
        criticalSet->setColor(QColor(231, 76, 60));
        
        QBarSet* warningSet = new QBarSet("Warning");
        warningSet->setColor(QColor(241, 196, 15));
        
        QStringList categories;
        
        // Generate hourly alert counts for last 12 hours
        QRandomGenerator* rng = QRandomGenerator::global();
        for (int i = 0; i < 12; i++) {
            int hour = 12 - i;
            categories << QString("-%1h").arg(hour);
            
            *criticalSet << rng->bounded(10);
            *warningSet << rng->bounded(15);
        }
        
        series->append(criticalSet);
        series->append(warningSet);
        chart->addSeries(series);
        
        QBarCategoryAxis* axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setLabelsColor(m_textColor);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("Alert Count");
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignTop);
    }
    
    // Update Component Comparison Chart (Horizontal Bar)
    {
        QChart* chart = m_comparisonChart->chart();
        chart->removeAllSeries();
        
        QBarSeries* series = new QBarSeries();
        
        QBarSet* healthSet = new QBarSet("Current Health");
        healthSet->setColor(QColor(46, 204, 113));
        
        QStringList categories;
        int count = 0;
        
        for (auto it = m_componentData.begin(); it != m_componentData.end() && count < 8; ++it) {
            categories << it.key();
            *healthSet << it->currentHealth;
            count++;
        }
        
        series->append(healthSet);
        chart->addSeries(series);
        
        QBarCategoryAxis* axisY = new QBarCategoryAxis();
        axisY->append(categories);
        axisY->setLabelsColor(m_textColor);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        
        QValueAxis* axisX = new QValueAxis();
        axisX->setRange(0, 100);
        axisX->setTitleText("Health (%)");
        axisX->setLabelsColor(m_textColor);
        axisX->setGridLineColor(m_gridColor);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        
        chart->legend()->setVisible(false);
    }
    
    // Update System Efficiency Chart (Area with gradient)
    {
        QChart* chart = m_efficiencyChart->chart();
        chart->removeAllSeries();
        
        QLineSeries* upperSeries = new QLineSeries();
        QLineSeries* lowerSeries = new QLineSeries();
        
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        QRandomGenerator* rng = QRandomGenerator::global();
        
        qreal efficiency = 85.0;
        for (int i = 0; i < 50; i++) {
            qint64 timestamp = currentTime - (50 - i) * 120000; // Last 100 minutes
            efficiency += (rng->bounded(1000) / 1000.0 - 0.5) * 5;
            efficiency = qBound(70.0, efficiency, 98.0);
            
            upperSeries->append(timestamp, efficiency);
            lowerSeries->append(timestamp, 0);
        }
        
        QAreaSeries* areaSeries = new QAreaSeries(upperSeries, lowerSeries);
        areaSeries->setName("Efficiency");
        areaSeries->setColor(QColor(155, 89, 182, 150));
        areaSeries->setBorderColor(QColor(155, 89, 182));
        
        chart->addSeries(areaSeries);
        
        QValueAxis* axisX = new QValueAxis();
        axisX->setTitleText("Time");
        axisX->setLabelsColor(m_textColor);
        axisX->setGridLineColor(m_gridColor);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setRange(0, 100);
        axisY->setTitleText("Efficiency (%)");
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        
        chart->setAxisX(axisX, areaSeries);
        chart->setAxisY(axisY, areaSeries);
        
        chart->legend()->setVisible(false);
    }
    
    // Update Uptime Chart (Stacked Bar)
    {
        QChart* chart = m_upTimeChart->chart();
        chart->removeAllSeries();
        
        QBarSeries* series = new QBarSeries();
        series->setBarWidth(0.8);
        
        QBarSet* uptimeSet = new QBarSet("Uptime");
        uptimeSet->setColor(QColor(46, 204, 113));
        
        QBarSet* downtimeSet = new QBarSet("Downtime");
        downtimeSet->setColor(QColor(231, 76, 60));
        
        QStringList categories;
        int count = 0;
        
        QRandomGenerator* rng = QRandomGenerator::global();
        for (auto it = m_componentData.begin(); it != m_componentData.end() && count < 6; ++it) {
            categories << it.key();
            
            qreal uptime = 90.0 + rng->bounded(10);
            *uptimeSet << uptime;
            *downtimeSet << (100.0 - uptime);
            count++;
        }
        
        series->append(uptimeSet);
        series->append(downtimeSet);
        chart->addSeries(series);
        
        QBarCategoryAxis* axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setLabelsColor(m_textColor);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setRange(0, 100);
        axisY->setTitleText("Percentage (%)");
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignTop);
    }
}

void AnalyticsDashboard::applyChartTheme(QChart* chart)
{
    chart->setBackgroundBrush(QBrush(m_chartBgColor));
    chart->setTitleBrush(QBrush(m_textColor));
    
    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);
    
    if (chart->legend()) {
        chart->legend()->setLabelColor(m_textColor);
        chart->legend()->setFont(QFont("Arial", 9));
    }
    
    chart->setMargins(QMargins(10, 10, 10, 10));
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
        data.type = "Unknown";
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
        data.type = type;
        data.currentHealth = 100.0;
        data.currentStatus = "EXCELLENT";
        data.totalMessages = 0;
        data.alertCount = 0;
        data.lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
        m_componentData[componentId] = data;
        
        m_componentTypeCount[type]++;
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
    m_componentFilterCombo->addItem("All Components");
    updateCharts();
}

void AnalyticsDashboard::refreshDashboard()
{
    updateCharts();
}

void AnalyticsDashboard::onThemeChanged()
{
    // Recreate UI with new theme colors
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
    
    // Reapply theme to all charts
    updateCharts();
}
