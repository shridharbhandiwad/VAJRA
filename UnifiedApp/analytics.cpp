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
    bool dark = tm.isDark();

    // Color tokens
    QString bg          = dark ? "#10131a" : "#f8fafc";
    QString panelBg     = dark ? "#161b26" : "#ffffff";
    QString borderCol   = dark ? "#1e2a3d" : "#e2e8f0";
    QString textPrimary = dark ? "#e2e8f0" : "#1e293b";
    QString textMuted   = dark ? "#64748b" : "#94a3b8";
    QString accent      = "#3B82F6";
    QString accentGreen = "#10B981";
    QString accentYellow= "#F59E0B";
    QString accentRed   = "#EF4444";
    QString accentCyan  = "#06B6D4";

    QString html;
    html += QString(
        "<style>"
        "* { box-sizing: border-box; margin: 0; padding: 0; }"
        "body { background: %1; color: %2; font-family: 'Inter','Segoe UI','Roboto',sans-serif; font-size: 12px; }"

        // Section label
        ".sec-label {"
        "  font-size: 9px; font-weight: 700; letter-spacing: 1.2px; text-transform: uppercase;"
        "  color: %3; margin: 14px 0 6px 0; padding: 0 12px;"
        "}"

        // Card base
        ".card {"
        "  background: %4; border: 1px solid %5;"
        "  border-radius: 8px; margin: 0 8px 6px 8px; padding: 10px 12px;"
        "}"

        // Fleet health band
        ".health-ring {"
        "  display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px;"
        "}"
        ".health-num {"
        "  font-size: 26px; font-weight: 800; line-height: 1;"
        "}"
        ".health-track {"
        "  background: %5; border-radius: 4px; height: 6px; margin: 6px 0;"
        "  overflow: hidden;"
        "}"
        ".health-fill {"
        "  height: 6px; border-radius: 4px;"
        "}"

        // Status dot row
        ".dot-row { display: flex; flex-wrap: wrap; gap: 4px; margin-top: 4px; }"
        ".dot-badge {"
        "  display: inline-flex; align-items: center; gap: 3px;"
        "  padding: 2px 7px; border-radius: 12px; font-size: 9px; font-weight: 700;"
        "}"

        // Stat mini row
        ".stat-grid {"
        "  display: table; width: 100%%; border-collapse: separate; border-spacing: 0 1px;"
        "}"
        ".stat-row { display: table-row; }"
        ".stat-k { display: table-cell; color: %3; font-size: 10px; padding: 3px 0; width: 60%%; }"
        ".stat-v { display: table-cell; font-size: 10px; font-weight: 700; color: %2; text-align: right; }"

        // Component card
        ".comp-card {"
        "  background: %4; border: 1px solid %5;"
        "  border-radius: 8px; margin: 0 8px 5px 8px; overflow: hidden;"
        "}"
        ".comp-header {"
        "  display: flex; align-items: center; justify-content: space-between;"
        "  padding: 8px 10px 6px 10px;"
        "}"
        ".comp-toggle {"
        "  font-weight: 700; font-size: 11px; color: %2; text-decoration: none; flex: 1;"
        "}"
        ".comp-toggle:hover { color: %6; }"
        ".status-badge {"
        "  display: inline-block; padding: 2px 8px; border-radius: 10px;"
        "  font-size: 9px; font-weight: 700; letter-spacing: 0.4px; white-space: nowrap;"
        "}"
        ".comp-bar-track {"
        "  height: 3px; background: %5; margin: 0 10px 8px 10px; border-radius: 2px;"
        "}"
        ".comp-bar-fill { height: 3px; border-radius: 2px; }"
        ".comp-meta {"
        "  display: flex; gap: 12px; padding: 0 10px 8px 10px; font-size: 10px; color: %3;"
        "}"
        ".comp-meta span b { color: %2; font-weight: 600; }"

        // Subcomponent items
        ".sub-list { padding: 0 10px 8px 20px; border-top: 1px solid %5; }"
        ".sub-item {"
        "  display: flex; align-items: center; gap: 6px;"
        "  padding: 4px 0; font-size: 10px; color: %3; border-bottom: 1px solid %5;"
        "}"
        ".sub-item:last-child { border-bottom: none; }"
        ".sub-dot { width: 6px; height: 6px; border-radius: 3px; flex-shrink: 0; }"
        ".sub-name { color: %2; font-weight: 600; flex: 1; }"
        ".sub-type { color: %3; font-size: 9px; }"

        // Empty state
        ".empty {"
        "  text-align: center; padding: 40px 20px; color: %3;"
        "}"
        ".empty-icon { font-size: 28px; margin-bottom: 10px; }"
        ".empty-title { font-size: 13px; font-weight: 600; color: %2; margin-bottom: 6px; }"
        ".empty-sub { font-size: 11px; line-height: 1.5; }"
        "</style>"
    )
    .arg(bg, textPrimary, textMuted, panelBg, borderCol, accent);

    if (m_stats.isEmpty()) {
        html += QString(
            "<div class='empty'>"
            "  <div class='empty-icon'>&#9881;</div>"
            "  <div class='empty-title'>No Components</div>"
            "  <div class='empty-sub'>Drag components onto the canvas<br>or load a design file to begin.</div>"
            "</div>"
        );
    } else {
        // ── Aggregate stats ───────────────────────────────────────
        QMap<QString, int> typeCounts;
        int totalSubComponents = 0;
        int totalMessages = 0;
        int operational = 0, warning = 0, degraded = 0, critical = 0;
        double totalHealth = 0.0;
        int healthCount = 0;

        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it)
            typeCounts[it.value()]++;
        for (auto it = m_subComponents.begin(); it != m_subComponents.end(); ++it)
            totalSubComponents += it.value().size();
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
            totalMessages += it->messageCount;
            QString status = getHealthStatus(it->currentColor);
            if (status == "OPERATIONAL") operational++;
            else if (status == "WARNING")  warning++;
            else if (status == "DEGRADED") degraded++;
            else if (status == "CRITICAL") critical++;
            int score = colorToHealthScore(it->currentColor);
            if (score >= 0) { totalHealth += score; healthCount++; }
        }

        int totalComp = m_stats.size();
        double avgHealth = healthCount > 0 ? totalHealth / healthCount : 0.0;

        QString healthColor = avgHealth >= 80 ? accentGreen
                            : avgHealth >= 55 ? accentYellow
                            : avgHealth >= 30 ? "#f97316"
                            : accentRed;

        // ── Fleet Health card ────────────────────────────────────
        html += QString("<div class='sec-label'>Overview</div>");
        html += QString(
            "<div class='card'>"
            "  <div class='health-ring'>"
            "    <div>"
            "      <div style='font-size:9px;font-weight:700;letter-spacing:0.8px;color:%2;margin-bottom:2px;'>FLEET HEALTH</div>"
            "      <div class='health-num' style='color:%3;'>%4<span style='font-size:13px;'>%%</span></div>"
            "    </div>"
            "    <div style='text-align:right;'>"
            "      <div style='font-size:9px;color:%2;'>%5 components</div>"
            "      <div style='font-size:9px;color:%2;margin-top:2px;'>%6 msgs total</div>"
            "    </div>"
            "  </div>"
            "  <div class='health-track'>"
            "    <div class='health-fill' style='background:%3;width:%4%%;'></div>"
            "  </div>"
            "  <div class='dot-row'>"
            "    <span class='dot-badge' style='background:rgba(16,185,129,0.15);color:%7;'>%8 OK</span>"
            "    <span class='dot-badge' style='background:rgba(245,158,11,0.15);color:%9;'>%10 WARN</span>"
            "    <span class='dot-badge' style='background:rgba(249,115,22,0.15);color:#f97316;'>%11 DEG</span>"
            "    <span class='dot-badge' style='background:rgba(239,68,68,0.15);color:%12;'>%13 CRIT</span>"
            "  </div>"
            "</div>"
        )
        .arg(bg, textMuted, healthColor)
        .arg(qRound(avgHealth))
        .arg(totalComp).arg(totalMessages)
        .arg(accentGreen).arg(operational)
        .arg(accentYellow).arg(warning)
        .arg(degraded)
        .arg(accentRed).arg(critical);

        // ── Quick stats ──────────────────────────────────────────
        html += QString(
            "<div class='card'>"
            "  <div class='stat-grid'>"
            "    <div class='stat-row'><div class='stat-k'>Types</div><div class='stat-v'>%1</div></div>"
            "    <div class='stat-row'><div class='stat-k'>Subcomponents</div><div class='stat-v'>%2</div></div>"
            "    <div class='stat-row'><div class='stat-k'>Total Messages</div><div class='stat-v'>%3</div></div>"
            "  </div>"
            "</div>"
        )
        .arg(typeCounts.size()).arg(totalSubComponents).arg(totalMessages);

        // ── Component Status ─────────────────────────────────────
        html += QString("<div class='sec-label'>Components</div>");

        for (auto it = m_componentTypes.begin(); it != m_componentTypes.end(); ++it) {
            const QString& id = it.key();
            QString type = it.value();
            bool isExpanded = m_expandedComponents.contains(id);
            const QList<SubComponentInfo>& subs = m_subComponents.value(id);
            const ComponentStats& stats = m_stats.value(id);

            QColor pillBg = getHealthStatusColor(stats.currentColor);
            QString statusLabel = stats.messageCount > 0
                ? getHealthStatusLabel(stats.currentColor)
                : "NO DATA";
            int healthPct = stats.messageCount > 0
                ? qMax(0, colorToHealthScore(stats.currentColor)) : 0;
            QString barColor = stats.messageCount > 0 ? pillBg.name() : borderCol;

            // Badge colors
            QString badgeBg, badgeFg;
            if      (statusLabel == "OPERATIONAL") { badgeBg = "rgba(16,185,129,0.15)";  badgeFg = accentGreen; }
            else if (statusLabel == "WARNING")      { badgeBg = "rgba(245,158,11,0.15)";  badgeFg = accentYellow; }
            else if (statusLabel == "DEGRADED")     { badgeBg = "rgba(249,115,22,0.15)";  badgeFg = "#f97316"; }
            else if (statusLabel == "CRITICAL")     { badgeBg = "rgba(239,68,68,0.15)";   badgeFg = accentRed; }
            else                                    { badgeBg = "rgba(100,116,139,0.15)"; badgeFg = textMuted; }

            QString trendIcon = getTrendIndicator(stats.colorHistory);
            QString msgRate = formatMessageRate(stats);
            QString expandIcon = isExpanded ? "&#9660;" : "&#9658;";

            html += "<div class='comp-card'>";
            html += QString(
                "<div class='comp-header'>"
                "  <a href='%1' class='comp-toggle'>%2 %3</a>"
                "  %4"
                "  <span class='status-badge' style='background:%5;color:%6;margin-left:6px;'>%7</span>"
                "</div>"
            )
            .arg(id, expandIcon, id, trendIcon, badgeBg, badgeFg, statusLabel);

            html += QString(
                "<div class='comp-bar-track'>"
                "  <div class='comp-bar-fill' style='background:%1;width:%2%%;'></div>"
                "</div>"
            ).arg(barColor).arg(healthPct);

            html += QString(
                "<div class='comp-meta'>"
                "  <span>Type <b>%1</b></span>"
                "  <span>Msgs <b>%2</b></span>"
                "  <span>Rate <b>%3</b></span>"
            ).arg(type).arg(stats.messageCount).arg(msgRate);

            if (!subs.isEmpty())
                html += QString("  <span>Subs <b>%1</b></span>").arg(subs.size());
            html += "</div>";

            if (isExpanded && !subs.isEmpty()) {
                html += "<div class='sub-list'>";
                for (const auto& sub : subs) {
                    QString dotColor = accent;
                    if (sub.type == "Label")        dotColor = accentCyan;
                    else if (sub.type == "LineEdit") dotColor = accentGreen;
                    else if (sub.type == "Button")   dotColor = accentYellow;

                    html += QString(
                        "<div class='sub-item'>"
                        "  <div class='sub-dot' style='background:%1;'></div>"
                        "  <span class='sub-name'>%2</span>"
                        "  <span class='sub-type'>%3</span>"
                        "</div>"
                    ).arg(dotColor, sub.name, sub.type);
                }
                html += "</div>";
            }

            html += "</div>";
        }

        // ── Activity summary ──────────────────────────────────────
        int totalChanges = 0;
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it)
            totalChanges += it->colorChanges;

        html += QString("<div class='sec-label'>Activity</div>");
        html += QString(
            "<div class='card'>"
            "  <div class='stat-grid'>"
            "    <div class='stat-row'><div class='stat-k'>Health Changes</div><div class='stat-v'>%1</div></div>"
            "    <div class='stat-row'><div class='stat-k'>Avg Fleet Health</div>"
            "      <div class='stat-v' style='color:%2;'>%3%%</div>"
            "    </div>"
            "    <div class='stat-row'><div class='stat-k'>Systems OK / Total</div>"
            "      <div class='stat-v'>%4 / %5</div>"
            "    </div>"
            "  </div>"
            "</div>"
        )
        .arg(totalChanges)
        .arg(healthColor)
        .arg(qRound(avgHealth))
        .arg(operational)
        .arg(totalComp);
    }

    m_textBrowser->setHtml(html);
}
