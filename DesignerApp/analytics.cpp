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
    QString text = "Component Statistics:\n\n";
    
    if (m_stats.isEmpty()) {
        text += "No components on canvas";
    } else {
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
            const QString& id = it.key();
            const ComponentStats& stats = it.value();
            QString type = m_componentTypes.value(id, "Unknown");
            
            text += QString("ID: %1\n").arg(id);
            text += QString("Type: %1\n").arg(type);
            text += QString("Messages: %2\n").arg(stats.messageCount);
            text += QString("Current Color: %3\n").arg(stats.currentColor.isEmpty() ? "N/A" : stats.currentColor);
            text += QString("Current Size: %4\n").arg(stats.currentSize == 0 ? "N/A" : QString::number(stats.currentSize));
            text += QString("Color Changes: %5\n").arg(stats.colorChanges);
            text += QString("Size Changes: %6\n").arg(stats.sizeChanges);
            text += "\n";
        }
    }
    
    m_textEdit->setText(text);
}
