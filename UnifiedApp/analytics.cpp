#include "analytics.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QDateTime>
#include <cmath>

Analytics::Analytics(QWidget* parent)
    : QWidget(parent)
    , m_textBrowser(new QTextBrowser(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_textBrowser->setReadOnly(true);
    m_textBrowser->setMinimumWidth(200);
    m_textBrowser->setOpenLinks(false);

    connect(m_textBrowser, &QTextBrowser::anchorClicked, this, &Analytics::onLinkClicked);

    layout->addWidget(m_textBrowser);
    setLayout(layout);
}

void Analytics::addComponent(const QString& id, const QString& type)
{
    m_stats[id] = ComponentStats();
    m_componentTypes[id] = type;
    m_subComponents[id] = QList<SubComponentInfo>();
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
        m_stats[id] = ComponentStats();
        m_componentTypes[id] = "Unknown";
    }

    ComponentStats& stats = m_stats[id];
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    stats.messageCount++;

    if (stats.firstMessageTime == 0)
        stats.firstMessageTime = now;
    stats.lastMessageTime = now;

    if (stats.currentColor != color && !stats.currentColor.isEmpty()) {
        stats.colorChanges++;
    }
    stats.currentColor = color;

    // Keep last 10 colors for trend
    stats.colorHistory.append(color);
    if (stats.colorHistory.size() > 10)
        stats.colorHistory.removeFirst();

    if (stats.currentSize != size && stats.currentSize != 0) {
        stats.sizeChanges++;
    }
    stats.currentSize = size;

    // Keep last 10 sizes for trend
    stats.sizeHistory.append(size);
    if (stats.sizeHistory.size() > 10)
        stats.sizeHistory.removeFirst();

    updateDisplay();
}

void Analytics::addDesignSubComponent(const QString& parentId, const QString& subType)
{
    if (m_subComponents.contains(parentId)) {
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
    m_subComponents[id].clear();
    m_componentTypes[id] = type;
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

    if (m_expandedComponents.contains(componentId)) {
        m_expandedComponents.remove(componentId);
    } else {
        m_expandedComponents.insert(componentId);
    }

    updateDisplay();
}

int Analytics::colorToHealthScore(const QString& color) const
{
    QString c = color.toLower();
    if (c == "#00ff00") return 100;
    if (c == "#ffff00") return 75;
    if (c == "#ffa500") return 50;
    if (c == "#ff0000") return 10;
    if (c == "#808080") return 0;
    return -1;
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

QString Analytics::getHealthStatusLabel(const QString& color) const
{
    return getHealthStatus(color);
}

QColor Analytics::getHealthStatusColor(const QString& color) const
{
    QString c = color.toLower();
    if (c == "#00ff00") return QColor("#22c55e");   // green-500
    if (c == "#ffff00") return QColor("#eab308");   // yellow-500
    if (c == "#ffa500") return QColor("#f97316");   // orange-500
    if (c == "#ff0000") return QColor("#ef4444");   // red-500
    if (c == "#808080") return QColor("#6b7280");   // gray-500
    return QColor("#94a3b8");                        // slate-400
}

QString Analytics::getHealthBar(qreal health) const
{
    int filled = qRound(health / 10.0);
    int empty = 10 - filled;
    return QString("[%1%2] %3%")
        .arg(QString(filled, QChar(0x2588)))
        .arg(QString(empty, QChar(0x2591)))
        .arg(qRound(health));
}

QString Analytics::getTrendIndicator(const QList<QString>& colorHistory) const
{
    if (colorHistory.size() < 3)
        return "";

    // Compare first half vs second half avg health score
    int midpoint = colorHistory.size() / 2;
    double earlyAvg = 0, recentAvg = 0;
    int earlyCount = 0, recentCount = 0;

    for (int i = 0; i < midpoint; i++) {
        int s = colorToHealthScore(colorHistory[i]);
        if (s >= 0) { earlyAvg += s; earlyCount++; }
    }
    for (int i = midpoint; i < colorHistory.size(); i++) {
        int s = colorToHealthScore(colorHistory[i]);
        if (s >= 0) { recentAvg += s; recentCount++; }
    }

    if (earlyCount == 0 || recentCount == 0)
        return "";

    earlyAvg /= earlyCount;
    recentAvg /= recentCount;
    double delta = recentAvg - earlyAvg;

    if (delta > 5)  return "<span style='color:#22c55e;'>&#9650;</span>";  // improving
    if (delta < -5) return "<span style='color:#ef4444;'>&#9660;</span>";  // degrading
    return "<span style='color:#94a3b8;'>&#9644;</span>";                  // stable
}

QString Analytics::formatMessageRate(const ComponentStats& stats) const
{
    if (stats.messageCount < 2 || stats.firstMessageTime == 0)
        return "—";

    qint64 elapsed = stats.lastMessageTime - stats.firstMessageTime;
    if (elapsed <= 0)
        return "—";

    double ratePerSec = (stats.messageCount - 1) * 1000.0 / elapsed;
    if (ratePerSec >= 1.0)
        return QString("%1/s").arg(ratePerSec, 0, 'f', 1);
    return QString("%1/min").arg(ratePerSec * 60.0, 0, 'f', 1);
}

void Analytics::updateDisplay()
{
    ThemeManager& tm = ThemeManager::instance();

    QString html;
    html += tm.analyticsStyleBlock();

    // Extended styles
    html += QString(
        "<style>"
        "a.component-toggle { color: %1; text-decoration: none; cursor: pointer; }"
        "a.component-toggle:hover { color: %2; text-decoration: underline; }"
        ".expand-icon { display: inline-block; width: 12px; font-weight: bold; }"
        ".subcomponent-item { margin-left: 20px; font-size: 11px; padding: 2px 0; }"
        ".pill {"
        "   display: inline-block; padding: 1px 7px; border-radius: 9px;"
        "   font-size: 10px; font-weight: bold; letter-spacing: 0.4px;"
        "   margin-left: 4px;"
        "}"
        ".health-bar-bg {"
        "   display: inline-block; width: 80px; height: 6px;"
        "   background: %3; border-radius: 3px; vertical-align: middle;"
        "}"
        ".health-bar-fill {"
        "   display: inline-block; height: 6px; border-radius: 3px;"
        "   vertical-align: top;"
        "}"
        ".summary-grid {"
        "   display: table; width: 100%%; border-collapse: collapse; margin-bottom: 4px;"
        "}"
        ".summary-row { display: table-row; }"
        ".summary-cell {"
        "   display: table-cell; padding: 3px 6px; font-size: 11px;"
        "   border-bottom: 1px solid %4;"
        "}"
        ".kv-row { margin: 2px 0; font-size: 11px; }"
        "</style>")
        .arg(tm.accentPrimary().name())
        .arg(tm.accentPrimary().lighter(120).name())
        .arg(tm.borderColor().name())
        .arg(tm.borderSubtle().name());

    if (m_stats.isEmpty()) {
        html += "<div class='header'>SYSTEM OVERVIEW</div>";
        html += "<div class='subheader'>No components on canvas</div>";
        html += "<br><div class='stat'>Drag components to the canvas or load a design file.</div>";
    } else {
        // ── Aggregate stats ───────────────────────────────────────
        QMap<QString, int> typeCounts;
        int totalSubComponents = 0;
        int totalMessages = 0;
        int operational = 0, warning = 0, degraded = 0, critical = 0, unknown = 0;
        double totalHealth = 0.0;
        int healthCount = 0;

        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            typeCounts[it.value()]++;
        }
        for (auto it = m_subComponents.begin(); it != m_subComponents.end(); ++it) {
            totalSubComponents += it.value().size();
        }
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
            totalMessages += it->messageCount;
            QString status = getHealthStatus(it->currentColor);
            if (status == "OPERATIONAL") { operational++; }
            else if (status == "WARNING") { warning++; }
            else if (status == "DEGRADED") { degraded++; }
            else if (status == "CRITICAL") { critical++; }
            else { unknown++; }
            int score = colorToHealthScore(it->currentColor);
            if (score >= 0) { totalHealth += score; healthCount++; }
        }

        int totalComp = m_stats.size();
        double avgHealth = healthCount > 0 ? totalHealth / healthCount : 0.0;

        // ── Header ────────────────────────────────────────────────
        html += "<div class='header'>SYSTEM OVERVIEW</div>";

        // System health summary card
        QString avgHealthColor = avgHealth >= 80 ? "#22c55e"
                               : avgHealth >= 55 ? "#eab308"
                               : avgHealth >= 30 ? "#f97316"
                               : "#ef4444";

        html += QString(
            "<div class='component' style='margin-bottom:6px;'>"
            "  <div style='display:flex; justify-content:space-between; align-items:center; margin-bottom:4px;'>"
            "    <span style='font-size:11px; font-weight:bold; color:%1;'>FLEET HEALTH</span>"
            "    <span style='font-size:17px; font-weight:bold; color:%2;'>%3%</span>"
            "  </div>"
            "  <div style='background:%4; height:8px; border-radius:4px; margin-bottom:6px;'>"
            "    <div style='background:%2; width:%3%%; height:8px; border-radius:4px;'></div>"
            "  </div>"
            "  <div style='font-size:10px; color:%5;'>"
            "    <span style='color:#22c55e;'>&#9679; %6 OK</span> &nbsp;"
            "    <span style='color:#eab308;'>&#9679; %7 WARN</span> &nbsp;"
            "    <span style='color:#f97316;'>&#9679; %8 DEG</span> &nbsp;"
            "    <span style='color:#ef4444;'>&#9679; %9 CRIT</span>"
            "  </div>"
            "</div>")
            .arg(tm.accentPrimary().name())
            .arg(avgHealthColor)
            .arg(qRound(avgHealth))
            .arg(tm.borderColor().name())
            .arg(tm.mutedText().name())
            .arg(operational).arg(warning).arg(degraded).arg(critical);

        // Summary counters row
        html += QString(
            "<div class='stat' style='margin-bottom:4px;'>"
            "  Components: <span class='count'>%1</span> &nbsp;"
            "  Types: <span class='count'>%2</span> &nbsp;"
            "  Subcomponents: <span class='count'>%3</span> &nbsp;"
            "  Messages: <span class='count'>%4</span>"
            "</div>")
            .arg(totalComp).arg(typeCounts.size())
            .arg(totalSubComponents).arg(totalMessages);

        // ── Type breakdown ────────────────────────────────────────
        html += "<div class='header'>BY TYPE</div>";
        for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it) {
            int pct = totalComp > 0 ? qRound(100.0 * it.value() / totalComp) : 0;
            html += QString(
                "<div class='stat' style='display:flex; justify-content:space-between;'>"
                "  <span>%1</span>"
                "  <span>"
                "    <span class='count'>%2</span>"
                "    <span style='color:%3; font-size:10px;'> (%4%%)</span>"
                "  </span>"
                "</div>")
                .arg(it.key()).arg(it.value()).arg(tm.mutedText().name()).arg(pct);
        }
        html += "<br>";

        // ── Component list ────────────────────────────────────────
        html += "<div class='header'>COMPONENT STATUS</div>";

        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            const QString& id = it.key();
            QString type = it.value();
            bool isExpanded = m_expandedComponents.contains(id);
            const QList<SubComponentInfo>& subs = m_subComponents.value(id);
            const ComponentStats& stats = m_stats.value(id);

            // Determine health color/status
            QColor pillBg = getHealthStatusColor(stats.currentColor);
            QString pillBgHex = pillBg.name();
            QString pillFg = "#ffffff";
            QString statusLabel = stats.messageCount > 0
                ? getHealthStatusLabel(stats.currentColor)
                : "NO DATA";
            int healthPct = stats.messageCount > 0
                ? qMax(0, colorToHealthScore(stats.currentColor))
                : 0;
            QString healthBarColor = pillBgHex;
            if (statusLabel == "NO DATA") healthBarColor = tm.borderColor().name();

            QString trendIcon = getTrendIndicator(stats.colorHistory);
            QString msgRate = formatMessageRate(stats);

            html += "<div class='component'>";

            // Header row: toggle arrow + id + status pill + trend
            QString expandIcon = isExpanded ? "&#9660;" : "&#9658;";
            html += QString(
                "<div style='display:flex; align-items:center; justify-content:space-between; margin-bottom:3px;'>"
                "  <a href='%1' class='component-toggle'>"
                "    <span class='expand-icon'>%2</span> <strong>%3</strong>"
                "  </a>"
                "  <span>"
                "    <span class='pill' style='background:%4; color:%5;'>%6</span>"
                "    %7"
                "  </span>"
                "</div>")
                .arg(id).arg(expandIcon).arg(id)
                .arg(pillBgHex).arg(pillFg).arg(statusLabel)
                .arg(trendIcon);

            // Health bar
            html += QString(
                "<div style='background:%1; height:5px; border-radius:3px; margin-bottom:5px;'>"
                "  <div style='background:%2; width:%3%%; height:5px; border-radius:3px;'></div>"
                "</div>")
                .arg(tm.borderColor().name())
                .arg(healthBarColor)
                .arg(healthPct);

            // Stats row
            html += QString(
                "<div style='display:flex; justify-content:space-between; font-size:10px; color:%1; margin-bottom:3px;'>"
                "  <span>Type: <span style='color:%2;'>%3</span></span>"
                "  <span>Msgs: <span style='color:%2;'>%4</span></span>"
                "  <span>Rate: <span style='color:%2;'>%5</span></span>"
                "  <span>&#916;Color: <span style='color:%2;'>%6</span></span>"
                "</div>")
                .arg(tm.mutedText().name())
                .arg(tm.primaryText().name())
                .arg(type).arg(stats.messageCount)
                .arg(msgRate).arg(stats.colorChanges);

            // Subcomponent count
            if (!subs.isEmpty()) {
                html += QString("<div class='stat' style='font-size:10px;'>Subcomponents: <span class='count'>%1</span></div>")
                    .arg(subs.size());
            } else {
                html += QString("<div class='stat' style='color:%1; font-size:10px;'>No subcomponents</div>")
                    .arg(tm.mutedText().name());
            }

            // Expanded subcomponents
            if (isExpanded && !subs.isEmpty()) {
                html += "<div style='margin-top:6px;'>";
                // Group by type for a cleaner view
                QMap<QString, int> subTypeCounts;
                for (const auto& sub : subs) {
                    subTypeCounts[sub.type]++;
                }
                // Show each subcomponent
                for (const auto& sub : subs) {
                    QString subTypeColor = tm.accentPrimary().name();
                    if (sub.type == "Label")       subTypeColor = tm.accentSecondary().name();
                    else if (sub.type == "LineEdit") subTypeColor = tm.accentSuccess().name();
                    else if (sub.type == "Button")   subTypeColor = tm.accentWarning().name();

                    html += QString(
                        "<div class='subcomponent-item'>"
                        "  &#8226; <span style='color:%1;'>%2</span> "
                        "  <span style='color:%3; font-size:10px;'>(%4)</span>"
                        "</div>")
                        .arg(subTypeColor).arg(sub.name)
                        .arg(tm.mutedText().name()).arg(sub.type);
                }
                html += "</div>";
            }

            html += "</div>";
        }

        // ── Activity summary ──────────────────────────────────────
        html += "<div class='header' style='margin-top:8px;'>ACTIVITY SUMMARY</div>";
        html += QString(
            "<div class='component' style='font-size:10px;'>"
            "  <div class='kv-row' style='display:flex; justify-content:space-between;'>"
            "    <span style='color:%1;'>Total Messages</span>"
            "    <span class='count'>%2</span>"
            "  </div>"
            "  <div class='kv-row' style='display:flex; justify-content:space-between;'>"
            "    <span style='color:%1;'>Health Changes</span>"
            "    <span class='count'>%3</span>"
            "  </div>"
            "  <div class='kv-row' style='display:flex; justify-content:space-between;'>"
            "    <span style='color:%1;'>Avg Fleet Health</span>"
            "    <span style='color:%4; font-weight:bold;'>%5%%</span>"
            "  </div>"
            "  <div class='kv-row' style='display:flex; justify-content:space-between;'>"
            "    <span style='color:%1;'>Systems OK / Total</span>"
            "    <span class='count'>%6 / %7</span>"
            "  </div>"
            "</div>")
            .arg(tm.mutedText().name())
            .arg(totalMessages)
            .arg([&]() {
                int changes = 0;
                for (auto it = m_stats.begin(); it != m_stats.end(); ++it)
                    changes += it->colorChanges;
                return changes;
            }())
            .arg(avgHealthColor)
            .arg(qRound(avgHealth))
            .arg(operational)
            .arg(totalComp);
    }

    m_textBrowser->setHtml(html);
}
