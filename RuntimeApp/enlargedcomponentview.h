#ifndef ENLARGEDCOMPONENTVIEW_H
#define ENLARGEDCOMPONENTVIEW_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPainter>
#include <QTimer>
#include "component.h"

// ─── Subsystem health bar widget ───────────────────────────────
class SubsystemHealthBar : public QWidget
{
    Q_OBJECT
public:
    SubsystemHealthBar(const QString& name, int health, const QColor& color,
                       QWidget* parent = nullptr);
    void updateHealth(int health, const QColor& color);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_name;
    int m_health;
    QColor m_color;
};

// ─── Health trend chart for a single component ─────────────────
class HealthTrendChart : public QWidget
{
    Q_OBJECT
public:
    explicit HealthTrendChart(QWidget* parent = nullptr);
    void addDataPoint(qreal healthValue, const QColor& color);
    void setSubcomponents(const QList<SubcomponentInfo>& subs);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct DataPoint {
        qreal value;
        QColor color;
    };
    QList<DataPoint> m_dataPoints;
    QList<SubcomponentInfo> m_subcomponents;
    static const int MAX_POINTS = 60;
};

// ─── Main enlarged view for a component tab ────────────────────
class EnlargedComponentView : public QWidget
{
    Q_OBJECT
public:
    EnlargedComponentView(const QString& componentId, ComponentType type,
                          const QList<SubcomponentInfo>& subcomponents,
                          QWidget* parent = nullptr);
    ~EnlargedComponentView();

    void updateComponentHealth(const QColor& color, qreal size);
    QString getComponentId() const { return m_componentId; }
    ComponentType getComponentType() const { return m_componentType; }

private:
    void setupUI();
    QString healthStatusText(const QColor& color) const;

    QString m_componentId;
    ComponentType m_componentType;
    QColor m_currentColor;
    qreal m_currentSize;

    // Left panel – enlarged component
    QGraphicsView* m_componentView;
    QGraphicsScene* m_componentScene;
    Component* m_displayComponent;
    QList<SubsystemHealthBar*> m_subsystemBars;
    QLabel* m_healthStatusLabel;
    QLabel* m_healthValueLabel;

    // Right panel – analytics
    HealthTrendChart* m_trendChart;
    QLabel* m_analyticsStatusLabel;
    QLabel* m_updateCountLabel;
    QLabel* m_statusChangesLabel;
    QLabel* m_avgHealthLabel;

    QList<SubcomponentInfo> m_subcomponents;
    int m_updateCount;
    int m_statusChanges;
    QString m_lastColor;
    qreal m_healthSum;
};

#endif // ENLARGEDCOMPONENTVIEW_H
