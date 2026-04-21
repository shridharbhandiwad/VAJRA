#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QWidget>
#include <QTextBrowser>
#include <QMap>
#include <QString>
#include <QSet>
#include <QList>
#include <QDateTime>

struct ComponentStats {
    int messageCount;
    QString currentColor;
    qreal currentSize;
    int colorChanges;
    int sizeChanges;

    // Rate & trend tracking
    qint64 firstMessageTime;   // ms since epoch
    qint64 lastMessageTime;    // ms since epoch
    QList<QString> colorHistory;  // last 10 colors for trend
    QList<qreal>   sizeHistory;   // last 10 sizes for trend

    ComponentStats()
        : messageCount(0), currentColor(""), currentSize(0),
          colorChanges(0), sizeChanges(0),
          firstMessageTime(0), lastMessageTime(0)
    {}
};

struct SubComponentInfo {
    QString name;
    QString type;  // "SubComponent", "Label", "LineEdit", "Button"

    SubComponentInfo() {}
    SubComponentInfo(const QString& n, const QString& t) : name(n), type(t) {}
};

/**
 * Analytics - System Overview panel showing components and their subcomponents.
 *
 * Displays a hierarchical view of all components on the canvas and their
 * subcomponents (both auto-created subsystems and user-added design widgets).
 * Components can be clicked to expand/collapse their subcomponent list.
 * Shows health status pills, message rates, trend indicators, and mini stats.
 */
class Analytics : public QWidget
{
    Q_OBJECT

public:
    explicit Analytics(QWidget* parent = nullptr);

    void addComponent(const QString& id, const QString& type);
    void removeComponent(const QString& id);
    void recordMessage(const QString& id, const QString& color, qreal size);

    /** Track a design sub-component being added to a parent component. */
    void addDesignSubComponent(const QString& parentId, const QString& subType);

    /** Track a regular sub-component being added to a parent component. */
    void addSubComponent(const QString& parentId, const QString& subName);

    /** Refresh a component's subcomponents list (called after editing). */
    void refreshComponent(const QString& id, const QString& type);

    void clear();
    void updateDisplay();

private slots:
    void onLinkClicked(const QUrl& url);

private:
    QString getHealthStatus(const QString& color) const;
    QString getHealthStatusLabel(const QString& color) const;
    QColor  getHealthStatusColor(const QString& color) const;
    QString getHealthBar(qreal health) const;
    QString getTrendIndicator(const QList<QString>& colorHistory) const;
    QString formatMessageRate(const ComponentStats& stats) const;
    int     colorToHealthScore(const QString& color) const;

    QTextBrowser* m_textBrowser;
    QMap<QString, ComponentStats> m_stats;
    QMap<QString, QString> m_componentTypes;

    // Subcomponent tracking (both SubComponents and DesignSubComponents)
    QMap<QString, QList<SubComponentInfo>> m_subComponents;  // parentId -> list of subcomponents

    // Expand/collapse state
    QSet<QString> m_expandedComponents;  // Set of component IDs that are currently expanded
};

#endif // ANALYTICS_H
