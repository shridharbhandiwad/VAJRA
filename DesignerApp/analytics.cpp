#include "analytics.h"
#include <QVBoxLayout>
#include <QLabel>

Analytics::Analytics(QWidget* parent)
    : QWidget(parent)
    , m_textEdit(new QTextEdit(this))
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

void Analytics::clear()
{
    m_stats.clear();
    m_componentTypes.clear();
    updateDisplay();
}

void Analytics::updateDisplay()
{
    QString text = "Radar Subsystem Count:\n\n";
    
    if (m_stats.isEmpty()) {
        text += "No subsystems on canvas";
    } else {
        QMap<QString, int> typeCounts;
        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            QString type = it.value();
            typeCounts[type]++;
        }
        
        text += QString("Total Subsystems: %1\n\n").arg(m_stats.size());
        
        for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it) {
            text += QString("%1: %2\n").arg(it.key()).arg(it.value());
        }
    }
    
    m_textEdit->setText(text);
}
