#include "analytics.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QUrl>

Analytics::Analytics(QWidget* parent)
    : QWidget(parent)
    , m_textBrowser(new QTextBrowser(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_textBrowser->setReadOnly(true);
    m_textBrowser->setMinimumWidth(200);
    m_textBrowser->setOpenLinks(false);  // We'll handle clicks ourselves
    
    connect(m_textBrowser, &QTextBrowser::anchorClicked, this, &Analytics::onLinkClicked);
    
    layout->addWidget(m_textBrowser);
    setLayout(layout);
}

void Analytics::addComponent(const QString& id, const QString& type)
{
    m_stats[id] = ComponentStats();
    m_componentTypes[id] = type;
    m_subComponents[id] = QList<SubComponentInfo>();  // Initialize empty subcomponents list
    updateDisplay();
}

void Analytics::removeComponent(const QString& id)
{
    m_stats.remove(id);
    m_componentTypes.remove(id);
    m_subComponents.remove(id);
    m_expandedComponents.remove(id);
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

void Analytics::addDesignSubComponent(const QString& parentId, const QString& subType)
{
    if (m_subComponents.contains(parentId)) {
        // Generate a unique name for this design subcomponent
        int count = 0;
        for (const auto& sub : m_subComponents[parentId]) {
            if (sub.type == subType) {
                count++;
            }
        }
        QString name = QString("%1_%2").arg(subType.toLower()).arg(count + 1);
        m_subComponents[parentId].append(SubComponentInfo(name, subType));
        updateDisplay();
    }
}

void Analytics::addSubComponent(const QString& parentId, const QString& subName)
{
    if (m_subComponents.contains(parentId)) {
        m_subComponents[parentId].append(SubComponentInfo(subName, "SubComponent"));
        updateDisplay();
    }
}

void Analytics::refreshComponent(const QString& id, const QString& type)
{
    // Clear existing subcomponents for this component
    m_subComponents[id].clear();
    
    // Update the component type
    m_componentTypes[id] = type;
    
    // Note: The actual subcomponents will be re-added by MainWindow
    // This method just prepares the analytics for a refresh
    updateDisplay();
}

void Analytics::clear()
{
    m_stats.clear();
    m_componentTypes.clear();
    m_subComponents.clear();
    m_expandedComponents.clear();
    updateDisplay();
}

void Analytics::onLinkClicked(const QUrl& url)
{
    QString componentId = url.toString();
    
    // Toggle expand/collapse state
    if (m_expandedComponents.contains(componentId)) {
        m_expandedComponents.remove(componentId);
    } else {
        m_expandedComponents.insert(componentId);
    }
    
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
    ThemeManager& tm = ThemeManager::instance();
    
    QString html;
    html += tm.analyticsStyleBlock();
    
    // Add custom styles for expand/collapse
    html += "<style>"
            "a.component-toggle { color: #7fb3d5; text-decoration: none; cursor: pointer; }"
            "a.component-toggle:hover { color: #9fc9e8; text-decoration: underline; }"
            ".expand-icon { display: inline-block; width: 12px; font-weight: bold; }"
            ".subcomponent-item { margin-left: 20px; font-size: 11px; padding: 2px 0; }"
            "</style>";
    
    if (m_stats.isEmpty()) {
        html += "<div class='header'>SYSTEM OVERVIEW</div>";
        html += "<div class='subheader'>No components on canvas</div>";
        html += "<br><div class='stat'>Drag components to the canvas or load a design file.</div>";
    } else {
        // Summary
        QMap<QString, int> typeCounts;
        int totalSubComponents = 0;
        
        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            typeCounts[it.value()]++;
        }
        
        for (auto it = m_subComponents.begin(); it != m_subComponents.end(); ++it) {
            totalSubComponents += it.value().size();
        }
        
        html += "<div class='header'>SYSTEM OVERVIEW</div>";
        html += QString("<div class='stat'>Components: <span class='count'>%1</span> &nbsp; "
                        "Types: <span class='count'>%2</span></div>")
                .arg(m_stats.size())
                .arg(typeCounts.size());
        html += QString("<div class='stat'>Total Subcomponents: <span class='count'>%1</span></div><br>")
                .arg(totalSubComponents);
        
        // Component type breakdown
        html += "<div class='header'>BY TYPE</div>";
        for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it) {
            html += QString("<div class='stat'>%1: <span class='count'>%2</span></div>")
                    .arg(it.key()).arg(it.value());
        }
        html += "<br>";
        
        // Component list with subcomponents
        html += "<div class='header'>COMPONENT STATUS</div>";
        
        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            const QString& id = it.key();
            QString type = it.value();
            bool isExpanded = m_expandedComponents.contains(id);
            const QList<SubComponentInfo>& subs = m_subComponents.value(id);
            
            html += "<div class='component'>";
            
            // Component name with expand/collapse icon (clickable)
            QString expandIcon = isExpanded ? "&#9660;" : "&#9658;";  // Down arrow : Right arrow
            html += QString("<a href='%1' class='component-toggle'>"
                            "<span class='expand-icon'>%2</span> <strong>%3</strong></a>")
                    .arg(id)
                    .arg(expandIcon)
                    .arg(id);
            
            html += QString("<div class='stat' style='margin-top: 3px;'>Type: %1</div>").arg(type);
            
            // Show subcomponent count
            if (!subs.isEmpty()) {
                html += QString("<div class='stat'>Subcomponents: <span class='count'>%1</span></div>")
                        .arg(subs.size());
            } else {
                html += "<div class='stat' style='color: #888;'>No subcomponents</div>";
            }
            
            // Show subcomponents if expanded
            if (isExpanded && !subs.isEmpty()) {
                html += "<div style='margin-top: 6px;'>";
                for (const auto& sub : subs) {
                    QString subTypeColor = "#7fb3d5";
                    if (sub.type == "Label") subTypeColor = "#88c0d0";
                    else if (sub.type == "LineEdit") subTypeColor = "#a3be8c";
                    else if (sub.type == "Button") subTypeColor = "#ebcb8b";
                    
                    html += QString("<div class='subcomponent-item'>"
                                    "&#8226; <span style='color: %1;'>%2</span> "
                                    "<span style='color: #888;'>(%3)</span>"
                                    "</div>")
                            .arg(subTypeColor)
                            .arg(sub.name)
                            .arg(sub.type);
                }
                html += "</div>";
            }
            
            html += "</div>";
        }
    }
    
    m_textBrowser->setHtml(html);
}
