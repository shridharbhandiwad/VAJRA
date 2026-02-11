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

QT_CHARTS_USE_NAMESPACE

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QLabel;
class QComboBox;
class QPushButton;
class QScrollArea;

// Chart type enumeration for dropdown selection
enum class ChartType {
    HealthTrend,
    ComponentDistribution,
    SubsystemPerformance,
    MessageFrequency,
    AlertHistory,
    ComponentComparison
};

/**
 * AnalyticsDashboard - Military-grade comprehensive data analytics dashboard
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
 *   - Component-wise filtering
 *   - 2x2 configurable grid layout with chart type selection
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
    
    // Chart update methods
    void updateAllCharts();          // Update all visible charts
    void updateKPIs();               // Update KPI cards
    void updateChart(QChartView* chartView, ChartType chartType, const QString& componentFilter);
    
    // Chart creation helpers - now with optional filtering
    void updateHealthTrendChart(QChartView* chartView, const QString& componentFilter = "");
    void updateComponentDistributionChart(QChartView* chartView, const QString& componentFilter = "");
    void updateSubsystemPerformanceChart(QChartView* chartView, const QString& componentFilter = "");
    void updateMessageFrequencyChart(QChartView* chartView, const QString& componentFilter = "");
    void updateAlertHistoryChart(QChartView* chartView, const QString& componentFilter = "");
    void updateComponentComparisonChart(QChartView* chartView, const QString& componentFilter = "");
    
    // KPI Cards
    QWidget* createKPICard(const QString& title, const QString& value, const QString& subtitle, const QColor& color);
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
    
    // 2x2 Grid structure
    struct ChartGrid {
        QChartView* chartView;
        QComboBox* chartTypeCombo;
        ChartType currentChartType;
        QWidget* containerWidget;
    };
    
    ChartGrid m_chartGrids[4];  // 2x2 grid = 4 charts
    
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
