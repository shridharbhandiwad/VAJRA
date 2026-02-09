#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QWidget>
#include <QTextEdit>
#include <QMap>
#include <QString>

struct ComponentStats {
    int messageCount;
    QString currentColor;
    qreal currentSize;
    int colorChanges;
    int sizeChanges;
    
    ComponentStats() : messageCount(0), currentColor(""), currentSize(0), 
                       colorChanges(0), sizeChanges(0) {}
};

/**
 * Analytics - Health analytics panel that works with any component type.
 * 
 * Fully dynamic - no hardcoded component types. Works with any type
 * registered in the ComponentRegistry.
 */
class Analytics : public QWidget
{
    Q_OBJECT
    
public:
    explicit Analytics(QWidget* parent = nullptr);
    
    void addComponent(const QString& id, const QString& type);
    void removeComponent(const QString& id);
    void recordMessage(const QString& id, const QString& color, qreal size);
    void clear();
    void updateDisplay();
    
private:
    QString getHealthStatus(const QString& color) const;
    QString getHealthBar(qreal health) const;
    
    QTextEdit* m_textEdit;
    QMap<QString, ComponentStats> m_stats;
    QMap<QString, QString> m_componentTypes;
};

#endif // ANALYTICS_H
