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
    , m_messageFreqChart(nullptr)
    , m_alertHistoryChart(nullptr)
    , m_comparisonChart(nullptr)
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
    setWindowTitle("Advanced Analytics Dashboard");
    resize(1400, 900);
    
    // Connect to theme changes
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &AnalyticsDashboard::onThemeChanged);
    
    setupUI();
    generateSampleData();
    updateKPIs();
    updateHealthTrendChart();  // Initialize real-time chart
    updateAllCharts();         // Initialize all other charts
    
    // Setup auto-refresh timer for ONLY the time-series chart
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &AnalyticsDashboard::updateHealthTrendChart);
    m_updateTimer->start(3000); // Update time-series every 3 seconds
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
    
    // Row 1: Health Trend (Real-Time) - Full width
    QWidget* row1Widget = new QWidget();
    QHBoxLayout* row1Layout = new QHBoxLayout(row1Widget);
    row1Layout->setSpacing(0);
    row1Layout->setContentsMargins(0, 0, 0, 0);
    row1Layout->addWidget(m_healthTrendChart);
    m_mainLayout->addWidget(row1Widget);
    
    // Row 2: Component Distribution and Subsystem Performance
    QWidget* row2Widget = new QWidget();
    QHBoxLayout* row2Layout = new QHBoxLayout(row2Widget);
    row2Layout->setSpacing(15);
    row2Layout->setContentsMargins(0, 0, 0, 0);
    row2Layout->addWidget(m_componentDistChart);
    row2Layout->addWidget(m_subsystemPerfChart);
    m_mainLayout->addWidget(row2Widget);
    
    // Row 3: Message Frequency and Alert History
    QWidget* row3Widget = new QWidget();
    QHBoxLayout* row3Layout = new QHBoxLayout(row3Widget);
    row3Layout->setSpacing(15);
    row3Layout->setContentsMargins(0, 0, 0, 0);
    row3Layout->addWidget(m_messageFreqChart);
    row3Layout->addWidget(m_alertHistoryChart);
    m_mainLayout->addWidget(row3Widget);
    
    // Row 4: Component Comparison - Full width
    QWidget* row4Widget = new QWidget();
    QHBoxLayout* row4Layout = new QHBoxLayout(row4Widget);
    row4Layout->setSpacing(0);
    row4Layout->setContentsMargins(0, 0, 0, 0);
    row4Layout->addWidget(m_comparisonChart);
    m_mainLayout->addWidget(row4Widget);
    
    m_scrollArea->setWidget(m_centralWidget);
    setCentralWidget(m_scrollArea);
}

QWidget* AnalyticsDashboard::createKPISection()
{
    QWidget* kpiWidget = new QWidget();
    QHBoxLayout* kpiLayout = new QHBoxLayout(kpiWidget);
    kpiLayout->setSpacing(20);
    kpiLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create KPI cards
    QWidget* card1 = createKPICard("Total Components", "0", "Monitored Systems", QColor(52, 152, 219));
    QWidget* card2 = createKPICard("Active Components", "0", "Currently Online", QColor(46, 204, 113));
    QWidget* card3 = createKPICard("Average Health", "0%", "System-Wide", QColor(241, 196, 15));
    QWidget* card4 = createKPICard("Total Alerts", "0", "Last 24 Hours", QColor(231, 76, 60));
    
    kpiLayout->addWidget(card1);
    kpiLayout->addWidget(card2);
    kpiLayout->addWidget(card3);
    kpiLayout->addWidget(card4);
    
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
    m_messageFreqChart = createMessageFrequencyChart();
    m_alertHistoryChart = createAlertHistoryChart();
    m_comparisonChart = createComponentComparisonChart();
}

QChartView* AnalyticsDashboard::createHealthTrendChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Component Health Trend (Real-Time Updates)");
    chart->setAnimationOptions(QChart::NoAnimation); // Disable animation for better performance
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(300);
    chartView->setMinimumWidth(400);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createComponentDistributionChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Component Type Distribution");
    chart->setAnimationOptions(QChart::NoAnimation);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(280);
    chartView->setMinimumWidth(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createSubsystemPerformanceChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Subsystem Performance");
    chart->setAnimationOptions(QChart::NoAnimation);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(280);
    chartView->setMinimumWidth(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createMessageFrequencyChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Message Frequency (Last 6 Hours)");
    chart->setAnimationOptions(QChart::NoAnimation);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(280);
    chartView->setMinimumWidth(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createAlertHistoryChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Alert History (Last 12 Hours)");
    chart->setAnimationOptions(QChart::NoAnimation);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(280);
    chartView->setMinimumWidth(350);
    
    return chartView;
}

QChartView* AnalyticsDashboard::createComponentComparisonChart()
{
    QChart* chart = new QChart();
    chart->setTitle("Component Health Comparison");
    chart->setAnimationOptions(QChart::NoAnimation);
    
    applyChartTheme(chart);
    
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(260);
    chartView->setMinimumWidth(400);
    
    return chartView;
}

void AnalyticsDashboard::generateSampleData()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 hourMs = 3600000; // 1 hour in milliseconds
    
    // Generate sample components
    QStringList componentTypes = {"Radar", "Antenna", "PowerSystem", "Communication", "CoolingUnit"};
    QStringList subsystems = {"Transmitter", "Receiver", "Processor"};
    
    QRandomGenerator* rng = QRandomGenerator::global();
    
    // Create 5-8 sample components (reduced from 8-12)
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
        
        // Generate health history - REDUCED from 288 to 60 points (last 1 hour, every minute)
        int numPoints = 60; // Much less data
        qreal baseHealth = 70.0 + rng->bounded(25);
        
        for (int j = 0; j < numPoints; j++) {
            qint64 timestamp = currentTime - (hourMs) + (j * 60000); // Last 1 hour
            
            // Add some variation
            qreal healthVariation = (rng->bounded(1000) / 1000.0 - 0.5) * 10;
            qreal health = qBound(50.0, baseHealth + healthVariation, 100.0);
            
            // Occasionally drop health to simulate issues
            if (rng->bounded(100) < 3) { // Reduced frequency
                health -= 15 + rng->bounded(15);
                data.alertCount++;
            }
            
            data.healthHistory.append(qMakePair(timestamp, health));
            
            // Add subsystem health data (reduced subsystems)
            for (const QString& subsys : subsystems) {
                qreal subsysHealth = health + (rng->bounded(1000) / 1000.0 - 0.5) * 10;
                subsysHealth = qBound(40.0, subsysHealth, 100.0);
                
                if (!data.subsystemHealth.contains(subsys)) {
                    data.subsystemHealth[subsys] = QVector<qreal>();
                }
                data.subsystemHealth[subsys].append(subsysHealth);
            }
            
            // Generate message timestamps (reduced frequency)
            if (rng->bounded(100) < 15) { // Reduced from 30% to 15%
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
    
    // Calculate KPI values
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
    
    // Update KPI labels
    if (m_totalComponentsLabel) m_totalComponentsLabel->setText(QString::number(totalComponents));
    if (m_activeComponentsLabel) m_activeComponentsLabel->setText(QString::number(activeComponents));
    if (m_avgHealthLabel) m_avgHealthLabel->setText(QString("%1%").arg(qRound(avgHealth)));
    if (m_totalAlertsLabel) m_totalAlertsLabel->setText(QString::number(totalAlerts));
}

void AnalyticsDashboard::updateHealthTrendChart()
{
    if (m_componentData.isEmpty() || !m_healthTrendChart) {
        return;
    }
    
    QChart* chart = m_healthTrendChart->chart();
    
    // Remove old series and clean up
    QList<QAbstractSeries*> oldSeries = chart->series();
    for (QAbstractSeries* series : oldSeries) {
        chart->removeSeries(series);
        delete series;
    }
    
    // Remove old axes
    QList<QAbstractAxis*> oldAxes = chart->axes();
    for (QAbstractAxis* axis : oldAxes) {
        chart->removeAxis(axis);
        delete axis;
    }
    
    QVector<QColor> seriesColors = {
        QColor(52, 152, 219),  // Blue
        QColor(46, 204, 113),  // Green
        QColor(241, 196, 15),  // Yellow
        QColor(231, 76, 60),   // Red
        QColor(155, 89, 182)   // Purple
    };
    
    int colorIndex = 0;
    int maxSeries = 4; // Limit to 4 components for better performance
    int seriesCount = 0;
    
    qint64 minTime = LLONG_MAX;
    qint64 maxTime = LLONG_MIN;
    qreal minHealth = 100.0;
    qreal maxHealth = 0.0;
    
    for (auto it = m_componentData.begin(); it != m_componentData.end() && seriesCount < maxSeries; ++it) {
        QLineSeries* series = new QLineSeries();
        series->setName(it.key());
        series->setColor(seriesColors[colorIndex % seriesColors.size()]);
        
        // Show all points (we already reduced data to 60 points)
        for (const auto& point : it->healthHistory) {
            qint64 timestamp = point.first;
            qreal health = point.second;
            
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
    axisX->setTitleText("Time (minutes ago)");
    axisX->setRange(minTime, maxTime);
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineColor(m_gridColor);
    axisX->setLabelFormat("%i");
    
    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Health (%)");
    axisY->setRange(qMax(0.0, minHealth - 10), qMin(100.0, maxHealth + 10));
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%i");
    
    for (QAbstractSeries* series : chart->series()) {
        chart->setAxisX(axisX, series);
        chart->setAxisY(axisY, series);
    }
    
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(m_textColor);
    
    // Update KPIs as well
    updateKPIs();
}

void AnalyticsDashboard::updateAllCharts()
{
    if (m_componentData.isEmpty()) {
        return;
    }
    
    // Update KPIs first
    updateKPIs();
    
    // Update Component Distribution Chart (Pie Chart)
    if (m_componentDistChart) {
        QChart* chart = m_componentDistChart->chart();
        
        // Clean up old series
        QList<QAbstractSeries*> oldSeries = chart->series();
        for (QAbstractSeries* series : oldSeries) {
            chart->removeSeries(series);
            delete series;
        }
        
        QPieSeries* series = new QPieSeries();
        
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
                QPieSlice* slice = series->append(it.key(), it.value());
                slice->setColor(pieColors[colorIndex % pieColors.size()]);
                slice->setLabelVisible(true);
                slice->setLabelColor(m_textColor);
                colorIndex++;
            }
        }
        
        series->setHoleSize(0.35); // Donut chart
        chart->addSeries(series);
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignRight);
        chart->legend()->setLabelColor(m_textColor);
    }
    
    // Update Subsystem Performance Chart (Bar Chart)
    if (m_subsystemPerfChart) {
        QChart* chart = m_subsystemPerfChart->chart();
        
        // Clean up old series and axes
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
    
    // Update Message Frequency Chart (Bar Chart)
    if (m_messageFreqChart) {
        QChart* chart = m_messageFreqChart->chart();
        
        // Clean up
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
        
        QBarSeries* series = new QBarSeries();
        QBarSet* set = new QBarSet("Messages");
        set->setColor(QColor(52, 152, 219));
        
        QStringList categories;
        for (auto it = m_componentData.begin(); it != m_componentData.end(); ++it) {
            categories << it.key();
            *set << it->totalMessages;
        }
        
        series->append(set);
        chart->addSeries(series);
        
        QBarCategoryAxis* axisX = new QBarCategoryAxis();
        axisX->append(categories);
        axisX->setLabelsColor(m_textColor);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);
        
        QValueAxis* axisY = new QValueAxis();
        axisY->setTitleText("Message Count");
        axisY->setLabelsColor(m_textColor);
        axisY->setGridLineColor(m_gridColor);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);
        
        chart->legend()->setVisible(false);
    }
    
    // Update Alert History Chart (Bar Chart)
    if (m_alertHistoryChart) {
        QChart* chart = m_alertHistoryChart->chart();
        
        // Clean up
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
        
        QBarSeries* series = new QBarSeries();
        QBarSet* set = new QBarSet("Alerts");
        set->setColor(QColor(231, 76, 60));
        
        QStringList categories;
        for (auto it = m_componentData.begin(); it != m_componentData.end(); ++it) {
            categories << it.key();
            *set << it->alertCount;
        }
        
        series->append(set);
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
        
        chart->legend()->setVisible(false);
    }
    
    // Update Component Comparison Chart (Horizontal Bar)
    if (m_comparisonChart) {
        QChart* chart = m_comparisonChart->chart();
        
        // Clean up
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
        
        QBarSeries* series = new QBarSeries();
        QBarSet* healthSet = new QBarSet("Health");
        
        QStringList categories;
        for (auto it = m_componentData.begin(); it != m_componentData.end(); ++it) {
            categories << it.key();
            qreal health = it->currentHealth;
            healthSet->append(health);
            
            // Color-code based on health
            QColor color = getHealthColor(health);
            // Note: Individual bar coloring would require separate sets
        }
        
        healthSet->setColor(QColor(46, 204, 113));
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
    updateAllCharts();
}

void AnalyticsDashboard::refreshDashboard()
{
    // Update all charts when refresh button is clicked
    updateAllCharts();
    // Also update the real-time chart
    updateHealthTrendChart();
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
    updateHealthTrendChart();
    updateAllCharts();
}
