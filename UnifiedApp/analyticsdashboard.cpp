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
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QProgressBar>
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
    , m_msgRateLabel(nullptr)
    , m_criticalCountLabel(nullptr)
    , m_avgHealthTrendLabel(nullptr)
    , m_alertsTrendLabel(nullptr)
    , m_summaryTable(nullptr)
    , m_timeRangeCombo(nullptr)
    , m_componentFilterCombo(nullptr)
    , m_refreshBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_updateTimer(nullptr)
    , m_prevAvgHealth(0.0)
    , m_prevAlertCount(0)
{
    setObjectName("AnalyticsDashboard");
    setWindowTitle("DATA ANALYTICS DASHBOARD");
    resize(1400, 900);

    for (int i = 0; i < 4; i++) {
        m_chartGrids[i].chartView = nullptr;
        m_chartGrids[i].chartTypeCombo = nullptr;
        m_chartGrids[i].containerWidget = nullptr;
    }

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &AnalyticsDashboard::onThemeChanged);

    setupUI();
    updateKPIs();
    updateAllCharts();

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &AnalyticsDashboard::updateAllCharts);
    m_updateTimer->start(3000);
}

AnalyticsDashboard::~AnalyticsDashboard()
{
    if (m_updateTimer)
        m_updateTimer->stop();
}

// ═══════════════════════════════════════════════════════════════════
//  UI Setup
// ═══════════════════════════════════════════════════════════════════

void AnalyticsDashboard::setupUI()
{
    ThemeManager& tm = ThemeManager::instance();

    m_bgColor = tm.windowBackground();
    m_textColor = tm.primaryText();
    m_gridColor = tm.chartGridLine();
    m_chartBgColor = tm.chartBackground();

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("dashboardScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_centralWidget = new QWidget();
    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(12);
    m_mainLayout->setContentsMargins(16, 12, 16, 16);

    // ── Header ───────────────────────────────────────────────────
    QWidget* headerWidget = new QWidget();
    headerWidget->setObjectName("dashboardHeader");
    QVBoxLayout* headerMainLayout = new QVBoxLayout(headerWidget);
    headerMainLayout->setSpacing(8);
    headerMainLayout->setContentsMargins(0, 0, 0, 0);

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

    QLabel* subtitleLabel = new QLabel("Real-time system monitoring, health analytics and performance insights");
    subtitleLabel->setObjectName("dashboardSubtitle");
    QFont subtitleFont;
    subtitleFont.setPointSize(9);
    subtitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    subtitleLabel->setFont(subtitleFont);

    // Controls row
    QHBoxLayout* controlsRow = new QHBoxLayout();
    controlsRow->setSpacing(12);

    QFont labelFont;
    labelFont.setPointSize(9);
    labelFont.setBold(true);
    labelFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    labelFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);

    QFont comboFont;
    comboFont.setPointSize(9);
    comboFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");

    QFont btnFont;
    btnFont.setPointSize(9);
    btnFont.setBold(true);
    btnFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    btnFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);

    QLabel* filterLabel = new QLabel("FILTER:");
    filterLabel->setObjectName("filterLabel");
    filterLabel->setFont(labelFont);

    m_componentFilterCombo = new QComboBox();
    m_componentFilterCombo->setObjectName("dashboardCombo");
    m_componentFilterCombo->addItem("ALL COMPONENTS");
    m_componentFilterCombo->setFixedWidth(180);
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

    // ── KPI Section (6 cards) ─────────────────────────────────────
    m_mainLayout->addWidget(createKPISection());

    // Divider
    QFrame* divider = new QFrame();
    divider->setObjectName("dashboardDivider");
    divider->setFrameShape(QFrame::HLine);
    divider->setFixedHeight(2);
    m_mainLayout->addWidget(divider);

    // ── 2×2 Charts Grid ──────────────────────────────────────────
    QGridLayout* chartsGrid = new QGridLayout();
    chartsGrid->setObjectName("chartsGrid");
    chartsGrid->setSpacing(12);
    chartsGrid->setContentsMargins(0, 4, 0, 0);

    m_chartGrids[0].containerWidget = createChartGrid(0, ChartType::HealthTrend);
    m_chartGrids[1].containerWidget = createChartGrid(1, ChartType::ComponentDistribution);
    m_chartGrids[2].containerWidget = createChartGrid(2, ChartType::SubsystemPerformance);
    m_chartGrids[3].containerWidget = createChartGrid(3, ChartType::AlertHistory);

    chartsGrid->addWidget(m_chartGrids[0].containerWidget, 0, 0);
    chartsGrid->addWidget(m_chartGrids[1].containerWidget, 0, 1);
    chartsGrid->addWidget(m_chartGrids[2].containerWidget, 1, 0);
    chartsGrid->addWidget(m_chartGrids[3].containerWidget, 1, 1);

    chartsGrid->setColumnStretch(0, 1);
    chartsGrid->setColumnStretch(1, 1);
    chartsGrid->setRowStretch(0, 1);
    chartsGrid->setRowStretch(1, 1);

    m_mainLayout->addLayout(chartsGrid);

    // ── Summary Table ─────────────────────────────────────────────
    QFrame* divider2 = new QFrame();
    divider2->setObjectName("dashboardDivider");
    divider2->setFrameShape(QFrame::HLine);
    divider2->setFixedHeight(2);
    m_mainLayout->addWidget(divider2);

    m_mainLayout->addWidget(createSummaryTable());

    m_scrollArea->setWidget(m_centralWidget);
    setCentralWidget(m_scrollArea);
}

// ═══════════════════════════════════════════════════════════════════
//  KPI Section — 6 cards in a row
// ═══════════════════════════════════════════════════════════════════

QWidget* AnalyticsDashboard::createKPISection()
{
    QWidget* kpiWidget = new QWidget();
    kpiWidget->setObjectName("kpiSection");
    QHBoxLayout* kpiLayout = new QHBoxLayout(kpiWidget);
    kpiLayout->setSpacing(10);
    kpiLayout->setContentsMargins(0, 0, 0, 0);

    QColor c1 = QColor(41, 128, 185);   // Blue
    QColor c2 = QColor(39, 174, 96);    // Green
    QColor c3 = QColor(243, 156, 18);   // Orange
    QColor c4 = QColor(231, 76, 60);    // Red
    QColor c5 = QColor(142, 68, 173);   // Purple
    QColor c6 = QColor(26, 188, 156);   // Teal

    kpiLayout->addWidget(createKPICard("COMPONENTS",  "0",  "Total Monitored",  c1));
    kpiLayout->addWidget(createKPICard("ACTIVE",      "0",  "Systems Online",   c2));
    kpiLayout->addWidget(createKPICard("AVG HEALTH",  "0%", "Fleet Average",    c3, ""));
    kpiLayout->addWidget(createKPICard("ALERTS",      "0",  "Total Warnings",   c4, ""));
    kpiLayout->addWidget(createKPICard("MSG RATE",    "0/s","Messages / sec",   c5));
    kpiLayout->addWidget(createKPICard("CRITICAL",    "0",  "Critical Systems", c6));

    return kpiWidget;
}

QWidget* AnalyticsDashboard::createKPICard(const QString& title, const QString& value,
                                            const QString& subtitle, const QColor& color,
                                            const QString& trendText)
{
    QWidget* card = new QWidget();
    card->setObjectName("kpiCard");
    card->setMinimumHeight(90);
    card->setMaximumHeight(115);

    QString gradientColor = color.lighter(110).name();
    card->setStyleSheet(QString(
        "QWidget#kpiCard { "
        "   border-left: 4px solid %1; "
        "   border-top: 1px solid %2; "
        "}"
    ).arg(color.name(), gradientColor));

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(3);
    cardLayout->setContentsMargins(12, 8, 12, 8);

    QLabel* titleLabel = new QLabel(title);
    titleLabel->setObjectName("kpiTitle");
    QFont titleFont;
    titleFont.setPointSize(8);
    titleFont.setBold(true);
    titleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet(QString("color: %1;").arg(color.name()));

    QLabel* valueLabel = new QLabel(value);
    valueLabel->setObjectName("kpiValue");
    QFont valueFont;
    valueFont.setPointSize(24);
    valueFont.setBold(true);
    valueFont.setWeight(QFont::Black);
    valueFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    valueFont.setLetterSpacing(QFont::AbsoluteSpacing, -1.0);
    valueLabel->setFont(valueFont);
    valueLabel->setAlignment(Qt::AlignCenter);

    // Wire up labels
    if (title.contains("COMPONENTS")) m_totalComponentsLabel = valueLabel;
    else if (title.contains("ACTIVE")) m_activeComponentsLabel = valueLabel;
    else if (title.contains("AVG HEALTH")) m_avgHealthLabel = valueLabel;
    else if (title.contains("ALERTS")) m_totalAlertsLabel = valueLabel;
    else if (title.contains("MSG RATE")) m_msgRateLabel = valueLabel;
    else if (title.contains("CRITICAL")) m_criticalCountLabel = valueLabel;

    // Trend label (only for avg health and alerts)
    QLabel* trendLabel = new QLabel(trendText.isEmpty() ? "" : trendText);
    trendLabel->setObjectName("kpiTrend");
    trendLabel->setAlignment(Qt::AlignCenter);
    QFont trendFont;
    trendFont.setPointSize(8);
    trendFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    trendLabel->setFont(trendFont);
    trendLabel->setVisible(!trendText.isNull());

    if (title.contains("AVG HEALTH")) m_avgHealthTrendLabel = trendLabel;
    else if (title.contains("ALERTS")) m_alertsTrendLabel = trendLabel;

    QLabel* subtitleLabel = new QLabel(subtitle);
    subtitleLabel->setObjectName("kpiSubtitle");
    QFont subtitleFont;
    subtitleFont.setPointSize(8);
    subtitleFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setAlignment(Qt::AlignCenter);

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
    if (!trendText.isNull())
        cardLayout->addWidget(trendLabel);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addWidget(progressBar);

    return card;
}

// ═══════════════════════════════════════════════════════════════════
//  Summary Table
// ═══════════════════════════════════════════════════════════════════

QWidget* AnalyticsDashboard::createSummaryTable()
{
    QWidget* container = new QWidget();
    container->setObjectName("summaryTableContainer");
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setSpacing(6);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel* heading = new QLabel("COMPONENT SUMMARY TABLE");
    heading->setObjectName("dashboardSubtitle");
    QFont hFont;
    hFont.setPointSize(10);
    hFont.setBold(true);
    hFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    hFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    heading->setFont(hFont);
    layout->addWidget(heading);

    m_summaryTable = new QTableWidget(0, 7);
    m_summaryTable->setObjectName("summaryTable");
    m_summaryTable->setHorizontalHeaderLabels({
        "Component", "Type", "Status", "Health %", "Messages", "Alerts", "Last Update"
    });
    m_summaryTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_summaryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_summaryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_summaryTable->verticalHeader()->setVisible(false);
    m_summaryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_summaryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summaryTable->setAlternatingRowColors(true);
    m_summaryTable->setMaximumHeight(220);
    m_summaryTable->setMinimumHeight(120);

    QFont tableFont;
    tableFont.setPointSize(9);
    tableFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    m_summaryTable->setFont(tableFont);

    layout->addWidget(m_summaryTable);
    return container;
}

void AnalyticsDashboard::updateSummaryTable()
{
    if (!m_summaryTable) return;

    m_summaryTable->setRowCount(0);

    QString componentFilter = m_componentFilterCombo->currentText();
    if (componentFilter == "ALL COMPONENTS")
        componentFilter = "";

    for (const auto& data : m_componentData) {
        if (!componentFilter.isEmpty() && data.componentId != componentFilter)
            continue;

        int row = m_summaryTable->rowCount();
        m_summaryTable->insertRow(row);

        // Component ID
        auto* idItem = new QTableWidgetItem(data.componentId);
        idItem->setFont(QFont("Inter, Segoe UI, Roboto, sans-serif", 9, QFont::Bold));
        m_summaryTable->setItem(row, 0, idItem);

        // Type
        m_summaryTable->setItem(row, 1, new QTableWidgetItem(data.type));

        // Status with color
        QString statusStr = getHealthStatus(data.currentHealth);
        auto* statusItem = new QTableWidgetItem(statusStr);
        QColor statusColor;
        if (statusStr == "EXCELLENT" || statusStr == "GOOD")
            statusColor = QColor("#22c55e");
        else if (statusStr == "FAIR")
            statusColor = QColor("#eab308");
        else if (statusStr == "POOR")
            statusColor = QColor("#f97316");
        else
            statusColor = QColor("#ef4444");
        statusItem->setForeground(QBrush(statusColor));
        statusItem->setFont(QFont("Inter, Segoe UI, Roboto, sans-serif", 9, QFont::Bold));
        m_summaryTable->setItem(row, 2, statusItem);

        // Health %
        auto* healthItem = new QTableWidgetItem(
            data.currentHealth > 0
                ? QString("%1%").arg(data.currentHealth, 0, 'f', 1)
                : "—"
        );
        healthItem->setForeground(QBrush(getHealthColor(data.currentHealth)));
        healthItem->setTextAlignment(Qt::AlignCenter);
        m_summaryTable->setItem(row, 3, healthItem);

        // Messages
        auto* msgItem = new QTableWidgetItem(QString::number(data.totalMessages));
        msgItem->setTextAlignment(Qt::AlignCenter);
        m_summaryTable->setItem(row, 4, msgItem);

        // Alerts
        auto* alertItem = new QTableWidgetItem(QString::number(data.alertCount));
        alertItem->setTextAlignment(Qt::AlignCenter);
        if (data.alertCount > 0)
            alertItem->setForeground(QBrush(QColor("#ef4444")));
        m_summaryTable->setItem(row, 5, alertItem);

        // Last Update
        QString lastUpdate = data.lastUpdateTime > 0
            ? QDateTime::fromMSecsSinceEpoch(data.lastUpdateTime).toString("hh:mm:ss")
            : "—";
        auto* timeItem = new QTableWidgetItem(lastUpdate);
        timeItem->setTextAlignment(Qt::AlignCenter);
        m_summaryTable->setItem(row, 6, timeItem);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Chart Grid Creation
// ═══════════════════════════════════════════════════════════════════

QWidget* AnalyticsDashboard::createChartGrid(int gridIndex, ChartType initialType)
{
    QWidget* container = new QWidget();
    container->setObjectName("chartContainer");
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 12, 12, 12);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);

    QComboBox* chartTypeCombo = new QComboBox();
    chartTypeCombo->setObjectName("chartTypeCombo");
    QFont comboFont;
    comboFont.setPointSize(9);
    comboFont.setBold(true);
    comboFont.setFamily("Inter, Segoe UI, Roboto, sans-serif");
    chartTypeCombo->setFont(comboFont);

    chartTypeCombo->addItem("Health Trend",           static_cast<int>(ChartType::HealthTrend));
    chartTypeCombo->addItem("Component Distribution", static_cast<int>(ChartType::ComponentDistribution));
    chartTypeCombo->addItem("Subsystem Performance",  static_cast<int>(ChartType::SubsystemPerformance));
    chartTypeCombo->addItem("Message Frequency",      static_cast<int>(ChartType::MessageFrequency));
    chartTypeCombo->addItem("Alert History",          static_cast<int>(ChartType::AlertHistory));
    chartTypeCombo->addItem("Component Comparison",   static_cast<int>(ChartType::ComponentComparison));
    chartTypeCombo->addItem("Health Heatmap",         static_cast<int>(ChartType::HealthHeatmap));
    chartTypeCombo->addItem("Uptime Timeline",        static_cast<int>(ChartType::UptimeTimeline));

    chartTypeCombo->setCurrentIndex(static_cast<int>(initialType));

    connect(chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, gridIndex](int) { this->onChartTypeChanged(gridIndex); });

    headerLayout->addWidget(chartTypeCombo, 1);

    QFrame* separator = new QFrame();
    separator->setObjectName("chartSeparator");
    separator->setFrameShape(QFrame::HLine);
    separator->setFixedHeight(1);

    QChart* chart = new QChart();
    applyChartTheme(chart);
    QChartView* chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRenderHint(QPainter::TextAntialiasing);
    chartView->setRenderHint(QPainter::SmoothPixmapTransform);
    chartView->setMinimumHeight(250);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    enableChartTooltips(chartView);

    layout->addLayout(headerLayout);
    layout->addWidget(separator);
    layout->addWidget(chartView, 1);

    m_chartGrids[gridIndex].chartView = chartView;
    m_chartGrids[gridIndex].chartTypeCombo = chartTypeCombo;
    m_chartGrids[gridIndex].currentChartType = initialType;
    m_chartGrids[gridIndex].containerWidget = container;

    return container;
}

// ═══════════════════════════════════════════════════════════════════
//  Slot handlers
// ═══════════════════════════════════════════════════════════════════

void AnalyticsDashboard::onChartTypeChanged(int gridIndex)
{
    if (gridIndex < 0 || gridIndex >= 4) return;

    ChartType newType = static_cast<ChartType>(
        m_chartGrids[gridIndex].chartTypeCombo->currentData().toInt()
    );

    m_chartGrids[gridIndex].currentChartType = newType;
    updateChartGrid(gridIndex, newType);
}

void AnalyticsDashboard::onComponentFilterChanged(int /*index*/)
{
    updateAllCharts();
}

void AnalyticsDashboard::updateChartGrid(int gridIndex, ChartType chartType)
{
    if (gridIndex < 0 || gridIndex >= 4) return;

    QChartView* chartView = m_chartGrids[gridIndex].chartView;
    if (!chartView) return;

    QString componentFilter = m_componentFilterCombo->currentText();
    if (componentFilter == "ALL COMPONENTS")
        componentFilter = "";

    updateChart(chartView, chartType, componentFilter);
}

void AnalyticsDashboard::updateChart(QChartView* chartView, ChartType chartType, const QString& componentFilter)
{
    switch (chartType) {
        case ChartType::HealthTrend:
            updateHealthTrendChart(chartView, componentFilter);       break;
        case ChartType::ComponentDistribution:
            updateComponentDistributionChart(chartView, componentFilter); break;
        case ChartType::SubsystemPerformance:
            updateSubsystemPerformanceChart(chartView, componentFilter);  break;
        case ChartType::MessageFrequency:
            updateMessageFrequencyChart(chartView, componentFilter);   break;
        case ChartType::AlertHistory:
            updateAlertHistoryChart(chartView, componentFilter);       break;
        case ChartType::ComponentComparison:
            updateComponentComparisonChart(chartView, componentFilter); break;
        case ChartType::HealthHeatmap:
            updateHealthHeatmapChart(chartView, componentFilter);      break;
        case ChartType::UptimeTimeline:
            updateUptimeTimelineChart(chartView, componentFilter);     break;
    }
}

void AnalyticsDashboard::createCharts()
{
    // Charts are created dynamically in createChartGrid
}

// ═══════════════════════════════════════════════════════════════════
//  Sample data (kept for testing)
// ═══════════════════════════════════════════════════════════════════

void AnalyticsDashboard::generateSampleData()
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    qint64 hourMs = 3600000;

    QStringList componentTypes = {"RADAR", "ANTENNA", "POWER_SYSTEM", "COMMUNICATION", "COOLING_UNIT"};
    QStringList subsystems = {"TRANSMITTER", "RECEIVER", "PROCESSOR"};

    QRandomGenerator* rng = QRandomGenerator::global();

    int numComponents = 5 + rng->bounded(4);
    for (int i = 0; i < numComponents; i++) {
        QString componentId = QString("COMP_%1").arg(i + 1, 3, 10, QChar('0'));
        QString type = componentTypes[rng->bounded(componentTypes.size())];

        ComponentHealthData data;
        data.componentId = componentId;
        data.type = type;
        data.totalMessages = 0;
        data.alertCount = 0;
        data.firstSeenTime = currentTime - hourMs;
        data.lastUpdateTime = currentTime;
        data.previousHealth = 0.0;

        int numPoints = 60;
        qreal baseHealth = 70.0 + rng->bounded(25);

        for (int j = 0; j < numPoints; j++) {
            qint64 timestamp = currentTime - hourMs + (j * 60000);
            qreal healthVariation = (rng->bounded(1000) / 1000.0 - 0.5) * 10;
            qreal health = qBound(50.0, baseHealth + healthVariation, 100.0);

            if (rng->bounded(100) < 3) {
                health -= 15 + rng->bounded(15);
                data.alertCount++;
            }

            data.healthHistory.append(qMakePair(timestamp, health));

            for (const QString& subsys : subsystems) {
                qreal subsysHealth = health + (rng->bounded(1000) / 1000.0 - 0.5) * 10;
                subsysHealth = qBound(40.0, subsysHealth, 100.0);
                if (!data.subsystemHealth.contains(subsys))
                    data.subsystemHealth[subsys] = QVector<qreal>();
                data.subsystemHealth[subsys].append(subsysHealth);
            }

            if (rng->bounded(100) < 15) {
                data.messageTimestamps.append(timestamp);
                data.totalMessages++;
            }
        }

        if (!data.healthHistory.isEmpty()) {
            data.previousHealth = data.healthHistory.size() > 1
                ? data.healthHistory[data.healthHistory.size() - 2].second
                : data.healthHistory.last().second;
            data.currentHealth = data.healthHistory.last().second;
            data.currentStatus = getHealthStatus(data.currentHealth);
        } else {
            data.currentHealth = baseHealth;
            data.previousHealth = baseHealth;
            data.currentStatus = getHealthStatus(baseHealth);
        }

        m_componentData[componentId] = data;
        m_componentTypeCount[type]++;
        m_componentFilterCombo->addItem(componentId);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  KPI update
// ═══════════════════════════════════════════════════════════════════

void AnalyticsDashboard::updateKPIs()
{
    if (m_componentData.isEmpty()) return;

    QString componentFilter = m_componentFilterCombo->currentText();
    if (componentFilter == "ALL COMPONENTS")
        componentFilter = "";

    int totalComponents = 0;
    int activeComponents = 0;
    int criticalComponents = 0;
    qreal totalHealth = 0.0;
    int totalAlerts = 0;
    qint64 totalMessages = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    for (const auto& data : m_componentData) {
        if (!componentFilter.isEmpty() && data.componentId != componentFilter)
            continue;

        totalComponents++;
        if (data.currentHealth > 50.0) activeComponents++;
        if (data.currentHealth > 0 && data.currentHealth < 40.0) criticalComponents++;
        totalHealth += data.currentHealth;
        totalAlerts += data.alertCount;
        totalMessages += data.totalMessages;
    }

    qreal avgHealth = totalComponents > 0 ? totalHealth / totalComponents : 0.0;

    // Message rate across all components (msgs / second, last 60s window)
    qint64 windowStart = now - 60000;
    qint64 recentMsgs = 0;
    for (const auto& data : m_componentData) {
        if (!componentFilter.isEmpty() && data.componentId != componentFilter)
            continue;
        for (qint64 ts : data.messageTimestamps) {
            if (ts >= windowStart) recentMsgs++;
        }
    }
    double msgRatePerSec = recentMsgs / 60.0;

    // Trend deltas
    double healthDelta = avgHealth - m_prevAvgHealth;
    int alertDelta = totalAlerts - m_prevAlertCount;

    if (m_totalComponentsLabel) m_totalComponentsLabel->setText(QString::number(totalComponents));
    if (m_activeComponentsLabel) m_activeComponentsLabel->setText(QString::number(activeComponents));
    if (m_avgHealthLabel)
        m_avgHealthLabel->setText(QString("%1%").arg(avgHealth, 0, 'f', 1));
    if (m_totalAlertsLabel) m_totalAlertsLabel->setText(QString::number(totalAlerts));
    if (m_criticalCountLabel) m_criticalCountLabel->setText(QString::number(criticalComponents));
    if (m_msgRateLabel)
        m_msgRateLabel->setText(msgRatePerSec >= 0.1
            ? QString("%1/s").arg(msgRatePerSec, 0, 'f', 1)
            : "0/s");

    // Health trend arrow
    if (m_avgHealthTrendLabel) {
        if (healthDelta > 1.0)
            m_avgHealthTrendLabel->setText(
                QString("<span style='color:#22c55e;'>&#9650; +%1%</span>").arg(healthDelta, 0, 'f', 1));
        else if (healthDelta < -1.0)
            m_avgHealthTrendLabel->setText(
                QString("<span style='color:#ef4444;'>&#9660; %1%</span>").arg(healthDelta, 0, 'f', 1));
        else
            m_avgHealthTrendLabel->setText(
                "<span style='color:#94a3b8;'>&#9644; stable</span>");
        m_avgHealthTrendLabel->setTextFormat(Qt::RichText);
    }

    // Alerts trend arrow
    if (m_alertsTrendLabel) {
        if (alertDelta > 0)
            m_alertsTrendLabel->setText(
                QString("<span style='color:#ef4444;'>&#9650; +%1</span>").arg(alertDelta));
        else if (alertDelta < 0)
            m_alertsTrendLabel->setText(
                QString("<span style='color:#22c55e;'>&#9660; %1</span>").arg(alertDelta));
        else
            m_alertsTrendLabel->setText(
                "<span style='color:#94a3b8;'>&#9644; no change</span>");
        m_alertsTrendLabel->setTextFormat(Qt::RichText);
    }

    m_prevAvgHealth = avgHealth;
    m_prevAlertCount = totalAlerts;
}

// ═══════════════════════════════════════════════════════════════════
//  Chart helpers
// ═══════════════════════════════════════════════════════════════════

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
    return QVector<QColor>{
        QColor(52,  152, 219),   // Bright Blue
        QColor(46,  204, 113),   // Emerald Green
        QColor(155, 89,  182),   // Amethyst Purple
        QColor(241, 196, 15),    // Sun Yellow
        QColor(231, 76,  60),    // Alizarin Red
        QColor(26,  188, 156),   // Turquoise
        QColor(230, 126, 34),    // Carrot Orange
        QColor(52,  73,  94),    // Wet Asphalt
        QColor(142, 68,  173),   // Wisteria
        QColor(22,  160, 133),   // Green Sea
        QColor(243, 156, 18),    // Orange
        QColor(189, 195, 199)    // Silver
    };
}

// ═══════════════════════════════════════════════════════════════════
//  Data ingestion
// ═══════════════════════════════════════════════════════════════════

void AnalyticsDashboard::recordComponentHealth(const QString& componentId, const QString& /*color*/,
                                                qreal health, qint64 timestamp)
{
    if (!m_componentData.contains(componentId)) {
        ComponentHealthData data;
        data.componentId = componentId;
        data.type = "UNKNOWN";
        data.currentHealth = 0.0;
        data.previousHealth = 0.0;
        data.currentStatus = "UNKNOWN";
        data.totalMessages = 0;
        data.alertCount = 0;
        data.firstSeenTime = timestamp;
        data.lastUpdateTime = timestamp;
        m_componentData[componentId] = data;
    }

    ComponentHealthData& data = m_componentData[componentId];
    data.previousHealth = data.currentHealth;
    data.healthHistory.append(qMakePair(timestamp, health));
    data.currentHealth = health;
    data.currentStatus = getHealthStatus(health);
    data.lastUpdateTime = timestamp;

    if (data.healthHistory.size() > 1000)
        data.healthHistory.remove(0, 500);

    if (health < 60)
        data.alertCount++;
}

void AnalyticsDashboard::recordSubsystemHealth(const QString& componentId, const QString& subsystem, qreal health)
{
    if (m_componentData.contains(componentId)) {
        m_componentData[componentId].subsystemHealth[subsystem].append(health);
        if (m_componentData[componentId].subsystemHealth[subsystem].size() > 500)
            m_componentData[componentId].subsystemHealth[subsystem].remove(0, 250);
    }
}

void AnalyticsDashboard::recordMessage(const QString& componentId, qint64 timestamp)
{
    if (m_componentData.contains(componentId)) {
        m_componentData[componentId].messageTimestamps.append(timestamp);
        m_componentData[componentId].totalMessages++;
        if (m_componentData[componentId].messageTimestamps.size() > 1000)
            m_componentData[componentId].messageTimestamps.remove(0, 500);
    }
}

void AnalyticsDashboard::addComponent(const QString& componentId, const QString& type)
{
    if (!m_componentData.contains(componentId)) {
        ComponentHealthData data;
        data.componentId = componentId;
        data.type = type.toUpper();
        data.currentHealth = 0.0;
        data.previousHealth = 0.0;
        data.currentStatus = "UNKNOWN";
        data.totalMessages = 0;
        data.alertCount = 0;
        data.firstSeenTime = QDateTime::currentMSecsSinceEpoch();
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
        if (m_componentTypeCount[type] <= 0)
            m_componentTypeCount.remove(type);

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
    updateAllCharts();
}

void AnalyticsDashboard::onThemeChanged()
{
    ThemeManager& tm = ThemeManager::instance();
    m_bgColor = tm.windowBackground();
    m_textColor = tm.primaryText();
    m_gridColor = tm.chartGridLine();
    m_chartBgColor = tm.chartBackground();
    updateAllCharts();
}

void AnalyticsDashboard::updateAllCharts()
{
    for (int i = 0; i < 4; i++) {
        if (m_chartGrids[i].chartView) {
            QString componentFilter = m_componentFilterCombo->currentText();
            if (componentFilter == "ALL COMPONENTS")
                componentFilter = "";
            updateChart(m_chartGrids[i].chartView, m_chartGrids[i].currentChartType, componentFilter);
        }
    }
    updateKPIs();
    updateSummaryTable();
}

// ═══════════════════════════════════════════════════════════════════
//  Individual chart updaters
// ═══════════════════════════════════════════════════════════════════

static void clearChart(QChartView* chartView)
{
    QChart* chart = chartView->chart();
    const auto oldSeries = chart->series();
    for (auto* s : oldSeries) { chart->removeSeries(s); delete s; }
    const auto oldAxes = chart->axes();
    for (auto* a : oldAxes) { chart->removeAxis(a); delete a; }
}

void AnalyticsDashboard::updateHealthTrendChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    clearChart(chartView);

    QChart* chart = chartView->chart();
    chart->setTitle("Component Health Trend");

    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) dataToShow = m_componentData;
    else if (m_componentData.contains(componentFilter))
        dataToShow[componentFilter] = m_componentData[componentFilter];

    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;

    qreal minX = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();

    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        if (it->healthHistory.isEmpty()) continue;

        QLineSeries* series = new QLineSeries();
        series->setName(it->componentId);

        for (const auto& point : it->healthHistory) {
            qreal x = static_cast<qreal>(point.first);
            series->append(x, point.second);
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
        }

        QColor lineColor = palette[colorIndex % palette.size()];
        QPen pen(lineColor);
        pen.setWidth(2);
        pen.setCapStyle(Qt::RoundCap);
        series->setPen(pen);
        series->setPointsVisible(it->healthHistory.size() < 20);

        chart->addSeries(series);
        colorIndex++;
    }

    QFont axisFont("Inter, Segoe UI, Roboto, sans-serif", 7);
    QFont axisTitleFont("Inter, Segoe UI, Roboto, sans-serif", 8);

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Time (epoch ms)");
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineColor(m_gridColor);
    axisX->setLabelFormat("%.0f");
    axisX->setLabelsFont(axisFont);
    axisX->setTitleFont(axisTitleFont);
    if (minX < maxX) axisX->setRange(minX, maxX);
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Health (%)");
    axisY->setRange(0, 105);
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f%%");
    axisY->setTickCount(11);
    axisY->setLabelsFont(axisFont);
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);

    for (auto* s : chart->series()) { s->attachAxis(axisX); s->attachAxis(axisY); }

    applyChartTheme(chart);
}

void AnalyticsDashboard::updateComponentDistributionChart(QChartView* chartView, const QString& /*componentFilter*/)
{
    if (!chartView) return;
    clearChart(chartView);

    QChart* chart = chartView->chart();
    chart->setTitle("Component Type Distribution");

    QPieSeries* pieSeries = new QPieSeries();
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;

    qreal totalCount = 0;
    for (auto it = m_componentTypeCount.begin(); it != m_componentTypeCount.end(); ++it)
        totalCount += it.value();

    for (auto it = m_componentTypeCount.begin(); it != m_componentTypeCount.end(); ++it) {
        if (it.value() <= 0) continue;

        qreal pct = (it.value() / totalCount) * 100.0;
        QString label = QString("%1\n%2 (%3%)")
            .arg(it.key()).arg(it.value()).arg(QString::number(pct, 'f', 1));

        QPieSlice* slice = pieSeries->append(label, it.value());
        QColor sliceColor = palette[colorIndex % palette.size()];
        slice->setColor(sliceColor);
        slice->setLabelVisible(true);
        slice->setLabelColor(m_textColor);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setBorderColor(m_chartBgColor);
        slice->setBorderWidth(2);
        slice->setExplodeDistanceFactor(0.06);

        QFont labelFont("Inter, Segoe UI, Roboto, sans-serif", 7);
        labelFont.setBold(true);
        slice->setLabelFont(labelFont);

        colorIndex++;
    }

    pieSeries->setHoleSize(0.45);
    pieSeries->setPieSize(0.82);
    chart->addSeries(pieSeries);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    applyChartTheme(chart);
}

void AnalyticsDashboard::updateSubsystemPerformanceChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    clearChart(chartView);

    QChart* chart = chartView->chart();
    chart->setTitle("Subsystem Performance");

    QMap<QString, qreal> subsystemAvgHealth;
    QMap<QString, int> subsystemCount;

    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) dataToShow = m_componentData;
    else if (m_componentData.contains(componentFilter))
        dataToShow[componentFilter] = m_componentData[componentFilter];

    for (const auto& compData : dataToShow) {
        for (auto it = compData.subsystemHealth.begin(); it != compData.subsystemHealth.end(); ++it) {
            if (it.value().isEmpty()) continue;
            qreal avg = 0.0;
            for (qreal v : it.value()) avg += v;
            avg /= it.value().size();
            subsystemAvgHealth[it.key()] += avg;
            subsystemCount[it.key()]++;
        }
    }

    QBarSeries* barSeries = new QBarSeries();
    QStringList categories;
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;

    for (auto it = subsystemAvgHealth.begin(); it != subsystemAvgHealth.end(); ++it) {
        if (subsystemCount[it.key()] <= 0) continue;
        qreal avgHealth = it.value() / subsystemCount[it.key()];

        QBarSet* set = new QBarSet(it.key());
        *set << avgHealth;
        set->setColor(palette[colorIndex % palette.size()]);
        set->setBorderColor(m_chartBgColor);
        barSeries->append(set);
        categories << it.key();
        colorIndex++;
    }

    barSeries->setBarWidth(0.75);
    chart->addSeries(barSeries);

    QFont axisFont("Inter, Segoe UI, Roboto, sans-serif", 7);
    QFont axisTitleFont("Inter, Segoe UI, Roboto, sans-serif", 8);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-30);
    axisX->setGridLineVisible(false);
    axisX->setLabelsFont(axisFont);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setRange(0, 105);
    axisY->setTitleText("Health (%)");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f%%");
    axisY->setTickCount(11);
    axisY->setLabelsFont(axisFont);
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

void AnalyticsDashboard::updateMessageFrequencyChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    clearChart(chartView);

    QChart* chart = chartView->chart();
    chart->setTitle("Message Frequency");

    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) dataToShow = m_componentData;
    else if (m_componentData.contains(componentFilter))
        dataToShow[componentFilter] = m_componentData[componentFilter];

    QBarSeries* barSeries = new QBarSeries();
    QStringList categories;
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;

    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        QBarSet* set = new QBarSet(it.key());
        *set << it->totalMessages;
        set->setColor(palette[colorIndex % palette.size()]);
        set->setBorderColor(m_chartBgColor);
        barSeries->append(set);
        categories << it.key();
        colorIndex++;
    }

    barSeries->setBarWidth(0.75);
    chart->addSeries(barSeries);

    QFont axisFont("Inter, Segoe UI, Roboto, sans-serif", 7);
    QFont axisTitleFont("Inter, Segoe UI, Roboto, sans-serif", 8);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-30);
    axisX->setGridLineVisible(false);
    axisX->setLabelsFont(axisFont);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Message Count");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f");
    axisY->setLabelsFont(axisFont);
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

void AnalyticsDashboard::updateAlertHistoryChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    clearChart(chartView);

    QChart* chart = chartView->chart();
    chart->setTitle("Alert History");

    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) dataToShow = m_componentData;
    else if (m_componentData.contains(componentFilter))
        dataToShow[componentFilter] = m_componentData[componentFilter];

    QBarSeries* barSeries = new QBarSeries();
    QStringList categories;
    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;

    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        QBarSet* set = new QBarSet(it.key());
        *set << it->alertCount;
        set->setColor(palette[(colorIndex + 4) % palette.size()]);
        set->setBorderColor(m_chartBgColor);
        barSeries->append(set);
        categories << it.key();
        colorIndex++;
    }

    barSeries->setBarWidth(0.75);
    chart->addSeries(barSeries);

    QFont axisFont("Inter, Segoe UI, Roboto, sans-serif", 7);
    QFont axisTitleFont("Inter, Segoe UI, Roboto, sans-serif", 8);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setLabelsColor(m_textColor);
    axisX->setLabelsAngle(-30);
    axisX->setGridLineVisible(false);
    axisX->setLabelsFont(axisFont);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Alert Count");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f");
    axisY->setLabelsFont(axisFont);
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

void AnalyticsDashboard::updateComponentComparisonChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    clearChart(chartView);

    QChart* chart = chartView->chart();
    chart->setTitle("Component Health Comparison");

    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) dataToShow = m_componentData;
    else if (m_componentData.contains(componentFilter))
        dataToShow[componentFilter] = m_componentData[componentFilter];

    QBarSeries* barSeries = new QBarSeries();
    QStringList categories;
    QVector<QColor> palette = getChartPalette();

    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        categories << it.key();
        qreal health = it->currentHealth;

        QBarSet* set = new QBarSet(it.key());
        set->append(health);

        QColor barColor;
        if (health >= 90)      barColor = palette[1];
        else if (health >= 75) barColor = palette[0];
        else if (health >= 60) barColor = palette[3];
        else                   barColor = palette[4];

        set->setColor(barColor);
        set->setBorderColor(m_chartBgColor);
        barSeries->append(set);
    }

    barSeries->setBarWidth(0.65);
    chart->addSeries(barSeries);

    QFont axisFont("Inter, Segoe UI, Roboto, sans-serif", 7);
    QFont axisTitleFont("Inter, Segoe UI, Roboto, sans-serif", 8);

    QBarCategoryAxis* axisY = new QBarCategoryAxis();
    axisY->append(categories);
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineVisible(false);
    axisY->setLabelsFont(axisFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    QValueAxis* axisX = new QValueAxis();
    axisX->setRange(0, 105);
    axisX->setTitleText("Health (%)");
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineColor(m_gridColor);
    axisX->setLabelFormat("%.0f%%");
    axisX->setTickCount(11);
    axisX->setLabelsFont(axisFont);
    axisX->setTitleFont(axisTitleFont);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    chart->legend()->setVisible(false);
    applyChartTheme(chart);
}

// ── Health Heatmap: status distribution across health buckets ──────

void AnalyticsDashboard::updateHealthHeatmapChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    clearChart(chartView);

    QChart* chart = chartView->chart();
    chart->setTitle("Health Status Distribution");

    // Bucket health values into 5 ranges: 0-20, 20-40, 40-60, 60-80, 80-100
    QStringList buckets = {"0-20%", "20-40%", "40-60%", "60-80%", "80-100%"};
    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) dataToShow = m_componentData;
    else if (m_componentData.contains(componentFilter))
        dataToShow[componentFilter] = m_componentData[componentFilter];

    QVector<int> counts(5, 0);
    for (const auto& compData : dataToShow) {
        for (const auto& point : compData.healthHistory) {
            qreal h = point.second;
            int bucket = qMin(4, static_cast<int>(h / 20.0));
            counts[bucket]++;
        }
    }

    QBarSeries* barSeries = new QBarSeries();
    QVector<QColor> heatColors = {
        QColor(231, 76, 60),    // Critical — red
        QColor(230, 126, 34),   // Poor — orange
        QColor(243, 156, 18),   // Fair — yellow
        QColor(46, 204, 113),   // Good — green
        QColor(26, 188, 156)    // Excellent — teal
    };

    for (int i = 0; i < 5; i++) {
        QBarSet* set = new QBarSet(buckets[i]);
        *set << counts[i];
        set->setColor(heatColors[i]);
        set->setBorderColor(m_chartBgColor);
        barSeries->append(set);
    }

    barSeries->setBarWidth(0.8);
    chart->addSeries(barSeries);

    QFont axisFont("Inter, Segoe UI, Roboto, sans-serif", 7);
    QFont axisTitleFont("Inter, Segoe UI, Roboto, sans-serif", 8);

    QBarCategoryAxis* axisX = new QBarCategoryAxis();
    axisX->append(buckets);
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineVisible(false);
    axisX->setLabelsFont(axisFont);
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("Data Points");
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f");
    axisY->setLabelsFont(axisFont);
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    applyChartTheme(chart);
}

// ── Uptime Timeline: health over time as stacked area ─────────────

void AnalyticsDashboard::updateUptimeTimelineChart(QChartView* chartView, const QString& componentFilter)
{
    if (!chartView) return;
    clearChart(chartView);

    QChart* chart = chartView->chart();
    chart->setTitle("Uptime Timeline (Health ≥ 60% = Up)");

    QMap<QString, ComponentHealthData> dataToShow;
    if (componentFilter.isEmpty()) dataToShow = m_componentData;
    else if (m_componentData.contains(componentFilter))
        dataToShow[componentFilter] = m_componentData[componentFilter];

    QVector<QColor> palette = getChartPalette();
    int colorIndex = 0;

    qreal minX = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();

    for (auto it = dataToShow.begin(); it != dataToShow.end(); ++it) {
        if (it->healthHistory.size() < 2) continue;

        // Create a binary 0/100 series: 100 when health >= 60 (up), 0 when down
        QLineSeries* series = new QLineSeries();
        series->setName(it->componentId);

        for (const auto& pt : it->healthHistory) {
            qreal x = static_cast<qreal>(pt.first);
            qreal y = (pt.second >= 60.0) ? 100.0 : 0.0;
            series->append(x, y);
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
        }

        QColor lineColor = palette[colorIndex % palette.size()];
        QPen pen(lineColor);
        pen.setWidth(2);
        series->setPen(pen);

        chart->addSeries(series);
        colorIndex++;
    }

    QFont axisFont("Inter, Segoe UI, Roboto, sans-serif", 7);
    QFont axisTitleFont("Inter, Segoe UI, Roboto, sans-serif", 8);

    QValueAxis* axisX = new QValueAxis();
    axisX->setTitleText("Time");
    axisX->setLabelsColor(m_textColor);
    axisX->setGridLineColor(m_gridColor);
    axisX->setLabelFormat("%.0f");
    axisX->setLabelsFont(axisFont);
    axisX->setTitleFont(axisTitleFont);
    if (minX < maxX) axisX->setRange(minX, maxX);
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis* axisY = new QValueAxis();
    axisY->setTitleText("State");
    axisY->setRange(-5, 110);
    axisY->setLabelsColor(m_textColor);
    axisY->setGridLineColor(m_gridColor);
    axisY->setLabelFormat("%.0f");
    axisY->setTickCount(3);
    axisY->setLabelsFont(axisFont);
    axisY->setTitleFont(axisTitleFont);
    chart->addAxis(axisY, Qt::AlignLeft);

    for (auto* s : chart->series()) { s->attachAxis(axisX); s->attachAxis(axisY); }

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    applyChartTheme(chart);
}

// ═══════════════════════════════════════════════════════════════════
//  Theme application
// ═══════════════════════════════════════════════════════════════════

void AnalyticsDashboard::applyChartTheme(QChart* chart)
{
    chart->setBackgroundBrush(QBrush(m_chartBgColor));
    chart->setTitleBrush(QBrush(m_textColor));
    chart->setBackgroundRoundness(8);
    chart->setAnimationOptions(QChart::AllAnimations);
    chart->setAnimationDuration(600);
    chart->setAnimationEasingCurve(QEasingCurve::OutCubic);

    QFont titleFont("Inter, Segoe UI, Roboto, sans-serif", 10, QFont::Bold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
    chart->setTitleFont(titleFont);

    if (chart->legend()) {
        chart->legend()->setLabelColor(m_textColor);
        chart->legend()->setAlignment(Qt::AlignBottom);
        chart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);
        chart->legend()->setShowToolTips(true);

        QFont legendFont("Inter, Segoe UI, Roboto, sans-serif", 7);
        chart->legend()->setFont(legendFont);
        chart->legend()->setBrush(QBrush(m_chartBgColor.lighter(102)));
        chart->legend()->setPen(QPen(m_gridColor.lighter(110), 1));
        chart->legend()->setBorderColor(m_gridColor.lighter(110));
    }

    chart->setMargins(QMargins(8, 10, 8, 8));
    chart->setDropShadowEnabled(false);
}

void AnalyticsDashboard::enableChartTooltips(QChartView* chartView)
{
    if (!chartView) return;
    chartView->setMouseTracking(true);
    chartView->setInteractive(true);
    chartView->setToolTip("Hover over data points for details");
}

// ═══════════════════════════════════════════════════════════════════
//  PDF Export
// ═══════════════════════════════════════════════════════════════════

void AnalyticsDashboard::onExportToPDF()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Dashboard to PDF", "analytics_dashboard.pdf",
        "PDF Files (*.pdf)");

    if (fileName.isEmpty()) return;

    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive))
        fileName += ".pdf";

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setPageSize(QPageSize(QPageSize::A4));

    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::warning(this, "Export Failed", "Unable to create PDF file.");
        return;
    }

    QRect pageRect = printer.pageRect(QPrinter::DevicePixel).toRect();
    QPixmap dashboardPixmap(m_centralWidget->size());
    m_centralWidget->render(&dashboardPixmap);

    QPixmap scaledPixmap = dashboardPixmap.scaled(pageRect.width(), pageRect.height(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int x = (pageRect.width()  - scaledPixmap.width())  / 2;
    int y = (pageRect.height() - scaledPixmap.height()) / 2;

    painter.drawPixmap(x, y, scaledPixmap);
    painter.end();

    QMessageBox::information(this, "Export Successful",
        QString("Dashboard exported to:\n%1").arg(fileName));
}
