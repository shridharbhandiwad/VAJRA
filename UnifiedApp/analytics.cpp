#include "analytics.h"
#include <QVBoxLayout>
#include <QLabel>

Analytics::Analytics(QWidget* parent)
    : QWidget(parent)
    , m_textEdit(new QTextEdit(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_textEdit->setReadOnly(true);
    m_textEdit->setMinimumWidth(200);
    
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
        // Auto-register unknown components that send health data
        m_stats[id] = ComponentStats();
        m_componentTypes[id] = "Unknown";
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

QString Analytics::getHealthStatus(const QString& color) const
{
    QString c = color.toLower();
    if (c == "#00ff00") return "OPERATIONAL";
    if (c == "#ffff00") return "WARNING";
    if (c == "#ffa500") return "DEGRADED";
    if (c == "#ff0000") return "CRITICAL";
    if (c == "#808080") return "OFFLINE";
    return "UNKNOWN";
}

QString Analytics::getHealthBar(qreal health) const
{
    int filled = qRound(health / 10.0);
    int empty = 10 - filled;
    return QString("[%1%2] %3%")
        .arg(QString(filled, QChar(0x2588)))  // Filled blocks
        .arg(QString(empty, QChar(0x2591)))    // Empty blocks
        .arg(qRound(health));
}

void Analytics::updateDisplay()
{
    QString html;
    html += "<style>"
            "body { color: #c4c7cc; font-family: 'Consolas', 'Monaco', monospace; font-size: 11px; }"
            ".header { color: #00BCD4; font-size: 12px; font-weight: bold; letter-spacing: 2px; }"
            ".subheader { color: #9aa0a6; font-size: 10px; margin-top: 4px; }"
            ".component { margin: 8px 0; padding: 8px; background: #1c1e26; border-radius: 4px; border-left: 3px solid #3a3f4b; }"
            ".component-name { color: #e8eaed; font-weight: bold; }"
            ".stat { color: #9aa0a6; font-size: 10px; }"
            ".operational { color: #4CAF50; }"
            ".warning { color: #FFC107; }"
            ".degraded { color: #FF9800; }"
            ".critical { color: #F44336; }"
            ".offline { color: #9E9E9E; }"
            ".count { color: #00BCD4; font-weight: bold; }"
            "</style>";
    
    if (m_stats.isEmpty()) {
        html += "<div class='header'>SYSTEM STATUS</div>";
        html += "<div class='subheader'>No subsystems registered</div>";
        html += "<br><div class='stat'>Drag components to the canvas or load a design file.</div>";
    } else {
        // Summary
        QMap<QString, int> typeCounts;
        int totalMessages = 0;
        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            typeCounts[it.value()]++;
        }
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
            totalMessages += it.value().messageCount;
        }
        
        html += "<div class='header'>SYSTEM OVERVIEW</div>";
        html += QString("<div class='stat'>Components: <span class='count'>%1</span> &nbsp; "
                        "Types: <span class='count'>%2</span> &nbsp; "
                        "Messages: <span class='count'>%3</span></div><br>")
                .arg(m_stats.size())
                .arg(typeCounts.size())
                .arg(totalMessages);
        
        // Type breakdown
        html += "<div class='header'>BY TYPE</div>";
        for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it) {
            html += QString("<div class='stat'>%1: <span class='count'>%2</span></div>")
                    .arg(it.key()).arg(it.value());
        }
        html += "<br>";
        
        // Per-component details
        html += "<div class='header'>COMPONENT STATUS</div>";
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
            const QString& id = it.key();
            const ComponentStats& stats = it.value();
            QString type = m_componentTypes.value(id, "Unknown");
            QString status = getHealthStatus(stats.currentColor);
            
            QString statusClass = "stat";
            if (status == "OPERATIONAL") statusClass = "operational";
            else if (status == "WARNING") statusClass = "warning";
            else if (status == "DEGRADED") statusClass = "degraded";
            else if (status == "CRITICAL") statusClass = "critical";
            else if (status == "OFFLINE") statusClass = "offline";
            
            html += "<div class='component'>";
            html += QString("<div class='component-name'>%1</div>").arg(id);
            html += QString("<div class='stat'>Type: %1</div>").arg(type);
            
            if (stats.messageCount > 0) {
                html += QString("<div class='%1'>Status: %2</div>")
                        .arg(statusClass).arg(status);
                html += QString("<div class='stat'>Health: %1</div>")
                        .arg(getHealthBar(stats.currentSize));
                html += QString("<div class='stat'>Updates: %1 | Changes: %2/%3</div>")
                        .arg(stats.messageCount)
                        .arg(stats.colorChanges)
                        .arg(stats.sizeChanges);
            } else {
                html += "<div class='stat'>Awaiting health data...</div>";
            }
            
            html += "</div>";
        }
    }
    
    m_textEdit->setHtml(html);
}
