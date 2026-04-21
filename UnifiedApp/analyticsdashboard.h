#ifndef ANALYTICSDASHBOARD_H
#define ANALYTICSDASHBOARD_H

#include <QMainWindow>
#include <QWidget>
#include <QMap>
#include <QVector>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QPieSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QPieSlice>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>

// Qt6: QtCharts types are in the global namespace (no separate QT_CHARTS_USE_NAMESPACE needed)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QComboBox;
class QPushButton;
class QScrollArea;
class QTableWidget;
class QProgressBar;

// Chart type enumeration for dropdown selection
enum class ChartType {
    HealthTrend,
    ComponentDistribution,
    SubsystemPerformance,
    MessageFrequency,
    AlertHistory,
    ComponentComparison,
    HealthHeatmap,
    UptimeTimeline
};

/**
 * AnalyticsDashboard - Comprehensive data analytics dashboard
 *
 * Features:
 *   - Real-time component health monitoring with time-series charts
 *   - Component distribution pie/donut charts
 *   - Subsystem performance bar charts
 *   - Health trend area charts
 *   - Message frequency scatter plots
 *   - Alert history and statistics
 *   - Health heatmap (status distribution over time buckets)
 *   - Uptime / connection-state timeline chart
 *   - Performance metrics and 6 KPI cards with trend deltas
 *   - Component-wise filtering
 *   - 2x2 configurable grid layout with chart type selection
 *   - Summary data table panel
 *   - PDF export capability
 *   - Enhanced tooltips and data visualization
 *   - Dark/Light theme support
 */
class AnalyticsDashboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit AnalyticsDashboard(QWidget* parent = nullptr);
    ~AnalyticsDashboard();

    // Data recording methods
    void recordComponentHealth(const QString& componentId, const QString& color, qreal health, qint64 timestamp);
    void recordSubsystemHealth(const QString& componentId, const QString& subsystem, qreal health);
    void recordMessage(const QString& componentId, qint64 timestamp);
    void addComponent(const QString& componentId, const QString& type);
    void removeComponent(const QString& componentId);
    void clear();

    // Dashboard refresh
    void refreshDashboard();

public slots:
    void onThemeChanged();

private slots:
    void onChartTypeChanged(int gridIndex);
    void onComponentFilterChanged(int index);
    void onExportToPDF();

private:
    void setupUI();
    void createCharts();
    void generateSampleData();

    // Chart grid management
    void updateChartGrid(int gridIndex, ChartType chartType);
    QWidget* createChartGrid(int gridIndex, ChartType initialType);

    // Summary table
    QWidget* createSummaryTable();
    void updateSummaryTable();

    // Chart update methods
    void updateAllCharts();
    void updateKPIs();
    void updateChart(QChartView* chartView, ChartType chartType, const QString& componentFilter);

    // Chart creation helpers
    void updateHealthTrendChart(QChartView* chartView, const QString& componentFilter = "");
    void updateComponentDistributionChart(QChartView* chartView, const QString& componentFilter = "");
    void updateSubsystemPerformanceChart(QChartView* chartView, const QString& componentFilter = "");
    void updateMessageFrequencyChart(QChartView* chartView, const QString& componentFilter = "");
    void updateAlertHistoryChart(QChartView* chartView, const QString& componentFilter = "");
    void updateComponentComparisonChart(QChartView* chartView, const QString& componentFilter = "");
    void updateHealthHeatmapChart(QChartView* chartView, const QString& componentFilter = "");
    void updateUptimeTimelineChart(QChartView* chartView, const QString& componentFilter = "");

    // KPI Cards (6 cards now)
    QWidget* createKPICard(const QString& title, const QString& value,
                           const QString& subtitle, const QColor& color,
                           const QString& trendText = "");
    QWidget* createKPISection();

    // Utility methods
    QColor getHealthColor(qreal health);
    QString getHealthStatus(qreal health);
    void applyChartTheme(QChart* chart);
    void enableChartTooltips(QChartView* chartView);
    QVector<QColor> getChartPalette() const;

    // Data structures
    struct ComponentHealthData {
        QString componentId;
        QString type;
        QVector<QPair<qint64, qreal>> healthHistory;    // timestamp, health%
        QMap<QString, QVector<qreal>> subsystemHealth;  // subsystem -> health values
        QVector<qint64> messageTimestamps;
        qreal currentHealth;
        qreal previousHealth;   // for delta calculation
        QString currentStatus;
        int totalMessages;
        int alertCount;
        qint64 firstSeenTime;
        qint64 lastUpdateTime;
    };

    QMap<QString, ComponentHealthData> m_componentData;
    QMap<QString, int> m_componentTypeCount;

    // UI Components
    QScrollArea* m_scrollArea;
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;

    // 2x2 Grid structure
    struct ChartGrid {
        QChartView* chartView;
        QComboBox* chartTypeCombo;
        ChartType currentChartType;
        QWidget* containerWidget;
    };

    ChartGrid m_chartGrids[4];  // 2x2 grid = 4 charts

    // 6 KPI Labels
    QLabel* m_totalComponentsLabel;
    QLabel* m_activeComponentsLabel;
    QLabel* m_avgHealthLabel;
    QLabel* m_totalAlertsLabel;
    QLabel* m_msgRateLabel;
    QLabel* m_criticalCountLabel;

    // KPI trend labels
    QLabel* m_avgHealthTrendLabel;
    QLabel* m_alertsTrendLabel;

    // Summary table
    QTableWidget* m_summaryTable;

    // Controls
    QComboBox* m_timeRangeCombo;
    QComboBox* m_componentFilterCombo;
    QPushButton* m_refreshBtn;
    QPushButton* m_exportBtn;

    // Timer for live updates
    QTimer* m_updateTimer;

    // Snapshot health averages for trend deltas
    qreal m_prevAvgHealth;
    int   m_prevAlertCount;

    // Theme colors
    QColor m_bgColor;
    QColor m_textColor;
    QColor m_gridColor;
    QColor m_chartBgColor;
};

#endif // ANALYTICSDASHBOARD_H
