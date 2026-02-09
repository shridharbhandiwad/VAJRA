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
            "body { color: #c4c7cc; font-family: 'Segoe UI', 'Roboto', sans-serif; font-size: 11px; margin: 0; padding: 0; }"
            ".header { color: #00BCD4; font-size: 12px; font-weight: bold; letter-spacing: 2px; margin-bottom: 6px; }"
            ".subheader { color: #9aa0a6; font-size: 10px; margin-top: 4px; }"
            ".overview-grid { margin: 4px 0 12px 0; }"
            ".overview-card { display: inline-block; background: rgba(18,20,26,0.9); border: 1px solid rgba(255,255,255,0.06); "
            "  border-radius: 6px; padding: 6px 10px; margin: 2px; }"
            ".overview-value { color: #00BCD4; font-size: 18px; font-weight: 700; }"
            ".overview-label { color: #5f6368; font-size: 8px; letter-spacing: 1px; text-transform: uppercase; }"
            ".component { margin: 6px 0; padding: 10px; background: rgba(18,20,26,0.8); border-radius: 8px; "
            "  border-left: 3px solid #3a3f4b; }"
            ".component-name { color: #e8eaed; font-weight: bold; font-size: 11px; margin-bottom: 4px; }"
            ".component-type { color: #5f6368; font-size: 9px; letter-spacing: 0.5px; }"
            ".stat { color: #9aa0a6; font-size: 10px; margin: 2px 0; }"
            ".operational { color: #4CAF50; font-weight: 600; }"
            ".warning { color: #FFC107; font-weight: 600; }"
            ".degraded { color: #FF9800; font-weight: 600; }"
            ".critical { color: #F44336; font-weight: 600; }"
            ".offline { color: #9E9E9E; font-weight: 600; }"
            ".count { color: #00BCD4; font-weight: bold; }"
            ".health-bar-bg { background: #2d313b; border-radius: 3px; height: 6px; margin: 3px 0; }"
            ".health-bar-fill { border-radius: 3px; height: 6px; }"
            ".type-item { color: #9aa0a6; font-size: 10px; padding: 2px 0; }"
            ".type-dot { display: inline-block; width: 6px; height: 6px; border-radius: 3px; margin-right: 6px; }"
            ".divider { border: none; border-top: 1px solid rgba(255,255,255,0.04); margin: 8px 0; }"
            "</style>";
    
    if (m_stats.isEmpty()) {
        html += "<div class='header'>SYSTEM STATUS</div>";
        html += "<div class='subheader'>No subsystems registered</div>";
        html += "<br><div class='stat'>Load a design file to begin monitoring.</div>";
    } else {
        // Summary stats
        QMap<QString, int> typeCounts;
        int totalMessages = 0;
        int healthyCount = 0;
        int warningCount = 0;
        int criticalCount = 0;
        
        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            typeCounts[it.value()]++;
        }
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
            totalMessages += it.value().messageCount;
            QString status = getHealthStatus(it.value().currentColor);
            if (status == "OPERATIONAL") healthyCount++;
            else if (status == "CRITICAL") criticalCount++;
            else if (status != "UNKNOWN") warningCount++;
        }
        
        // System overview cards
        html += "<div class='header'>SYSTEM OVERVIEW</div>";
        html += "<table cellspacing='4' cellpadding='0' width='100%'><tr>";
        html += QString("<td style='background: rgba(18,20,26,0.9); border: 1px solid rgba(255,255,255,0.06); "
                        "border-radius: 6px; padding: 6px 8px; text-align: center;'>"
                        "<div style='color: #00BCD4; font-size: 16px; font-weight: 700;'>%1</div>"
                        "<div style='color: #5f6368; font-size: 8px; letter-spacing: 1px;'>COMPONENTS</div></td>")
                .arg(m_stats.size());
        html += QString("<td style='background: rgba(18,20,26,0.9); border: 1px solid rgba(255,255,255,0.06); "
                        "border-radius: 6px; padding: 6px 8px; text-align: center;'>"
                        "<div style='color: #00BCD4; font-size: 16px; font-weight: 700;'>%1</div>"
                        "<div style='color: #5f6368; font-size: 8px; letter-spacing: 1px;'>TYPES</div></td>")
                .arg(typeCounts.size());
        html += QString("<td style='background: rgba(18,20,26,0.9); border: 1px solid rgba(255,255,255,0.06); "
                        "border-radius: 6px; padding: 6px 8px; text-align: center;'>"
                        "<div style='color: #00BCD4; font-size: 16px; font-weight: 700;'>%1</div>"
                        "<div style='color: #5f6368; font-size: 8px; letter-spacing: 1px;'>MESSAGES</div></td>")
                .arg(totalMessages);
        html += "</tr></table><br>";
        
        // Type breakdown
        html += "<div class='header'>BY TYPE</div>";
        for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it) {
            html += QString("<div class='type-item'>"
                           "<span style='color: #00BCD4;'>&#9679;</span> %1: "
                           "<span class='count'>%2</span></div>")
                    .arg(it.key()).arg(it.value());
        }
        html += "<hr class='divider'>";
        
        // Per-component details
        html += "<div class='header'>COMPONENT STATUS</div>";
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
            const QString& id = it.key();
            const ComponentStats& stats = it.value();
            QString type = m_componentTypes.value(id, "Unknown");
            QString status = getHealthStatus(stats.currentColor);
            
            QString statusClass = "stat";
            QString borderColor = "#3a3f4b";
            if (status == "OPERATIONAL") { statusClass = "operational"; borderColor = "#4CAF50"; }
            else if (status == "WARNING") { statusClass = "warning"; borderColor = "#FFC107"; }
            else if (status == "DEGRADED") { statusClass = "degraded"; borderColor = "#FF9800"; }
            else if (status == "CRITICAL") { statusClass = "critical"; borderColor = "#F44336"; }
            else if (status == "OFFLINE") { statusClass = "offline"; borderColor = "#9E9E9E"; }
            
            html += QString("<div class='component' style='border-left-color: %1;'>").arg(borderColor);
            html += QString("<div class='component-name'>%1</div>").arg(id);
            html += QString("<div class='component-type'>%1</div>").arg(type);
            
            if (stats.messageCount > 0) {
                html += QString("<div class='%1' style='margin: 3px 0;'>&#9679; %2</div>")
                        .arg(statusClass).arg(status);
                
                // Visual health bar
                int healthPct = qRound(stats.currentSize);
                QString barColor = borderColor;
                html += QString("<div style='background: #2d313b; border-radius: 3px; height: 6px; margin: 4px 0;'>"
                               "<div style='background: %1; border-radius: 3px; height: 6px; width: %2%;'></div>"
                               "</div>")
                        .arg(barColor).arg(healthPct);
                html += QString("<div class='stat'>Health: <span style='color: %1; font-weight: 600;'>%2%</span></div>")
                        .arg(barColor).arg(healthPct);
                html += QString("<div class='stat'>Updates: %1 &nbsp;|&nbsp; Changes: %2/%3</div>")
                        .arg(stats.messageCount)
                        .arg(stats.colorChanges)
                        .arg(stats.sizeChanges);
            } else {
                html += "<div class='stat' style='font-style: italic;'>Awaiting health data...</div>";
            }
            
            html += "</div>";
        }
    }
    
    m_textEdit->setHtml(html);
}
