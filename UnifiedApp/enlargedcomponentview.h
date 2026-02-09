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
#include "subcomponent.h"

// ─── Subsystem health bar widget ───────────────────────────────
class SubsystemHealthBar : public QWidget
{
    Q_OBJECT
public:
    SubsystemHealthBar(const QString& name, qreal health, const QColor& color,
                       QWidget* parent = nullptr);
    void updateHealth(qreal health, const QColor& color);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString m_name;
    qreal m_health;
    QColor m_color;
};

// ─── Health trend chart for a single component ─────────────────
class HealthTrendChart : public QWidget
{
    Q_OBJECT
public:
    explicit HealthTrendChart(QWidget* parent = nullptr);
    void addDataPoint(qreal healthValue, const QColor& color);
    void clearData();
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct DataPoint {
        qreal value;
        QColor color;
    };
    QList<DataPoint> m_dataPoints;
    static const int MAX_POINTS = 60;
};

// ─── Main enlarged view for a component tab ────────────────────
class EnlargedComponentView : public QWidget
{
    Q_OBJECT
public:
    EnlargedComponentView(const QString& componentId, const QString& typeId,
                          const QStringList& subcomponentNames,
                          QWidget* parent = nullptr);
    ~EnlargedComponentView();

    void updateComponentHealth(const QColor& color, qreal size);
    void updateSubcomponentHealth(const QString& subName, qreal health, const QColor& color);
    QString getComponentId() const { return m_componentId; }
    QString getTypeId() const { return m_typeId; }

private:
    void setupUI();
    QString healthStatusText(const QColor& color) const;

    QString m_componentId;
    QString m_typeId;
    QColor m_currentColor;
    qreal m_currentSize;

    // Left panel – enlarged component
    QGraphicsView* m_componentView;
    QGraphicsScene* m_componentScene;
    Component* m_displayComponent;
    QMap<QString, SubsystemHealthBar*> m_subsystemBars;
    QLabel* m_healthStatusLabel;
    QLabel* m_healthValueLabel;

    // Right panel – analytics
    HealthTrendChart* m_trendChart;
    QLabel* m_analyticsStatusLabel;
    QLabel* m_updateCountLabel;
    QLabel* m_statusChangesLabel;
    QLabel* m_avgHealthLabel;

    QStringList m_subcomponentNames;
    int m_updateCount;
    int m_statusChanges;
    QString m_lastColor;
    qreal m_healthSum;
};

#endif // ENLARGEDCOMPONENTVIEW_H
