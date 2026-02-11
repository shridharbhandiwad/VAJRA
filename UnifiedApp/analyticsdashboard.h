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

QT_CHARTS_USE_NAMESPACE

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QComboBox;
class QPushButton;
class QScrollArea;

/**
 * AnalyticsDashboard - Comprehensive data analytics dashboard with multiple visualization types
 * 
 * Features:
 *   - Real-time component health monitoring with time-series charts
 *   - Component distribution pie charts
 *   - Subsystem performance bar charts
 *   - Health trend area charts
 *   - Message frequency scatter plots
 *   - Telemetry data line charts
 *   - Alert history and statistics
 *   - Performance metrics and KPIs
 *   - Component comparison views
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
    
private:
    void setupUI();
    void createCharts();
    void generateSampleData();
    
    // Separate update methods for real-time vs on-demand
    void updateHealthTrendChart();  // Real-time only
    void updateAllCharts();          // On-demand (Refresh button)
    void updateKPIs();               // Update KPI cards
    
    // Chart creation helpers
    QChartView* createHealthTrendChart();
    QChartView* createComponentDistributionChart();
    QChartView* createSubsystemPerformanceChart();
    QChartView* createMessageFrequencyChart();
    QChartView* createAlertHistoryChart();
    QChartView* createComponentComparisonChart();
    
    // KPI Cards
    QWidget* createKPICard(const QString& title, const QString& value, const QString& subtitle, const QColor& color);
    QWidget* createKPISection();
    
    // Utility methods
    QColor getHealthColor(qreal health);
    QString getHealthStatus(qreal health);
    void applyChartTheme(QChart* chart);
    
    // Data structures
    struct ComponentHealthData {
        QString componentId;
        QString type;
        QVector<QPair<qint64, qreal>> healthHistory;  // timestamp, health
        QMap<QString, QVector<qreal>> subsystemHealth;  // subsystem -> health values
        QVector<qint64> messageTimestamps;
        qreal currentHealth;
        QString currentStatus;
        int totalMessages;
        int alertCount;
        qint64 lastUpdateTime;
    };
    
    QMap<QString, ComponentHealthData> m_componentData;
    QMap<QString, int> m_componentTypeCount;
    
    // UI Components
    QScrollArea* m_scrollArea;
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    
    // Chart views
    QChartView* m_healthTrendChart;      // Real-time
    QChartView* m_componentDistChart;    // On-demand
    QChartView* m_subsystemPerfChart;    // On-demand
    QChartView* m_messageFreqChart;      // On-demand
    QChartView* m_alertHistoryChart;     // On-demand
    QChartView* m_comparisonChart;       // On-demand
    
    // KPI Labels
    QLabel* m_totalComponentsLabel;
    QLabel* m_activeComponentsLabel;
    QLabel* m_avgHealthLabel;
    QLabel* m_totalAlertsLabel;
    
    // Controls
    QComboBox* m_timeRangeCombo;
    QComboBox* m_componentFilterCombo;
    QPushButton* m_refreshBtn;
    QPushButton* m_exportBtn;
    
    // Timer for live updates
    QTimer* m_updateTimer;
    
    // Theme colors
    QColor m_bgColor;
    QColor m_textColor;
    QColor m_gridColor;
    QColor m_chartBgColor;
};

#endif // ANALYTICSDASHBOARD_H
