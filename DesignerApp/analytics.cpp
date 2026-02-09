#include "analytics.h"
#include <QVBoxLayout>
#include <QLabel>

Analytics::Analytics(QWidget* parent)
    : QWidget(parent)
    , m_textEdit(new QTextEdit(this))
    , m_totalSubComponents(0)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel("Analytics", this);
    QFont font = titleLabel->font();
    font.setPointSize(12);
    font.setBold(true);
    titleLabel->setFont(font);
    
    m_textEdit->setReadOnly(true);
    m_textEdit->setMinimumWidth(200);
    
    layout->addWidget(titleLabel);
    layout->addWidget(m_textEdit);
    
    setLayout(layout);
}

void Analytics::addComponent(const QString& id, const QString& type)
{
    m_stats[id] = ComponentStats();
    m_componentTypes[id] = type;
    updateDisplay();
}

void Analytics::removeComponent(const QString& id)
{
    m_stats.remove(id);
    m_componentTypes.remove(id);
    updateDisplay();
}

void Analytics::recordMessage(const QString& id, const QString& color, qreal size)
{
    if (!m_stats.contains(id)) {
        return;
    }
    
    ComponentStats& stats = m_stats[id];
    stats.messageCount++;
    
    if (stats.currentColor != color && !stats.currentColor.isEmpty()) {
        stats.colorChanges++;
    }
    stats.currentColor = color;
    
    if (stats.currentSize != size && stats.currentSize != 0) {
        stats.sizeChanges++;
    }
    stats.currentSize = size;
    
    updateDisplay();
}

void Analytics::addSubComponent(const QString& parentId, const QString& subType)
{
    Q_UNUSED(parentId);
    m_totalSubComponents++;
    m_subTypeCounts[subType]++;
    updateDisplay();
}

void Analytics::clear()
{
    m_stats.clear();
    m_componentTypes.clear();
    m_totalSubComponents = 0;
    m_subTypeCounts.clear();
    updateDisplay();
}

void Analytics::updateDisplay()
{
    QString text = "Radar Subsystem Count:\n\n";
    
    if (m_stats.isEmpty() && m_totalSubComponents == 0) {
        text += "No subsystems on canvas";
    } else {
        // Component type counts
        QMap<QString, int> typeCounts;
        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            typeCounts[it.value()]++;
        }
        
        text += QString("Total Subsystems: %1\n").arg(m_stats.size());
        
        for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it) {
            text += QString("  %1: %2\n").arg(it.key()).arg(it.value());
        }
        
        // Sub-component counts
        if (m_totalSubComponents > 0) {
            text += QString("\nSub-Components: %1\n").arg(m_totalSubComponents);
            for (auto it = m_subTypeCounts.begin(); it != m_subTypeCounts.end(); ++it) {
                text += QString("  %1: %2\n").arg(it.key()).arg(it.value());
            }
        }
    }
    
    m_textEdit->setText(text);
}
