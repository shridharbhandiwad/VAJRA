#include "enlargedcomponentview.h"
#include <QPainter>
#include <QPainterPath>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include <QDebug>
#include <QtMath>

// ═══════════════════════════════════════════════════════════════
//  SubsystemHealthBar
// ═══════════════════════════════════════════════════════════════

SubsystemHealthBar::SubsystemHealthBar(const QString& name, int health,
                                       const QColor& color, QWidget* parent)
    : QWidget(parent), m_name(name), m_health(health), m_color(color)
{
    setMinimumHeight(36);
    setMaximumHeight(44);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SubsystemHealthBar::updateHealth(int health, const QColor& color)
{
    m_health = qBound(0, health, 100);
    m_color = color;
    update();
}

QSize SubsystemHealthBar::sizeHint() const
{
    return QSize(300, 40);
}

void SubsystemHealthBar::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int margin = 4;

    // Background card
    QColor cardBg(36, 39, 46);
    p.setPen(QPen(QColor(58, 63, 75), 1));
    p.setBrush(cardBg);
    p.drawRoundedRect(margin, 1, w - 2 * margin, h - 2, 6, 6);

    // Left colour indicator strip
    p.setPen(Qt::NoPen);
    p.setBrush(m_color);
    QPainterPath strip;
    strip.addRoundedRect(margin, 1, 5, h - 2, 3, 3);
    p.drawPath(strip);

    // Subsystem name
    p.setPen(QColor(200, 203, 210));
    p.setFont(QFont("Segoe UI", 9));
    QRectF nameRect(margin + 12, 0, w * 0.42, h);
    p.drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, m_name);

    // Health bar background
    int barX = w * 0.48;
    int barW = w * 0.32;
    int barH = 10;
    int barY = (h - barH) / 2;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(45, 49, 59));
    p.drawRoundedRect(barX, barY, barW, barH, 5, 5);

    // Health bar fill
    int fillW = barW * m_health / 100;
    if (fillW > 0) {
        QLinearGradient grad(barX, barY, barX + fillW, barY);
        grad.setColorAt(0, m_color.darker(120));
        grad.setColorAt(1, m_color);
        p.setBrush(grad);
        p.drawRoundedRect(barX, barY, fillW, barH, 5, 5);
    }

    // Health percentage
    p.setPen(m_color);
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    QRectF pctRect(w * 0.82, 0, w * 0.16, h);
    p.drawText(pctRect, Qt::AlignVCenter | Qt::AlignRight,
               QString("%1%").arg(m_health));
}

// ═══════════════════════════════════════════════════════════════
//  HealthTrendChart
// ═══════════════════════════════════════════════════════════════

HealthTrendChart::HealthTrendChart(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void HealthTrendChart::addDataPoint(qreal healthValue, const QColor& color)
{
    DataPoint dp;
    dp.value = qBound(0.0, healthValue, 100.0);
    dp.color = color;
    m_dataPoints.append(dp);
    if (m_dataPoints.size() > MAX_POINTS) {
        m_dataPoints.removeFirst();
    }
    update();
}

void HealthTrendChart::setSubcomponents(const QList<SubcomponentInfo>& subs)
{
    m_subcomponents = subs;
    update();
}

QSize HealthTrendChart::sizeHint() const
{
    return QSize(300, 200);
}

void HealthTrendChart::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Chart area
    int chartLeft = 40;
    int chartTop = 10;
    int chartRight = w - 15;
    int chartBottom = h - 25;
    int chartW = chartRight - chartLeft;
    int chartH = chartBottom - chartTop;

    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(28, 30, 38));
    p.drawRoundedRect(0, 0, w, h, 6, 6);

    // Grid lines
    p.setPen(QPen(QColor(50, 54, 65), 1, Qt::DotLine));
    for (int i = 0; i <= 4; i++) {
        int y = chartTop + chartH * i / 4;
        p.drawLine(chartLeft, y, chartRight, y);
        // Y-axis labels
        p.setPen(QColor(140, 143, 150));
        p.setFont(QFont("Segoe UI", 7));
        p.drawText(QRectF(0, y - 8, chartLeft - 4, 16),
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString("%1").arg(100 - 25 * i));
        p.setPen(QPen(QColor(50, 54, 65), 1, Qt::DotLine));
    }

    // X-axis label
    p.setPen(QColor(140, 143, 150));
    p.setFont(QFont("Segoe UI", 7));
    p.drawText(QRectF(chartLeft, chartBottom + 4, chartW, 18),
               Qt::AlignCenter, "Time (updates)");

    if (m_dataPoints.isEmpty()) {
        // Placeholder text
        p.setPen(QColor(100, 104, 115));
        p.setFont(QFont("Segoe UI", 10));
        p.drawText(QRectF(chartLeft, chartTop, chartW, chartH),
                   Qt::AlignCenter, "Waiting for health data...");
        return;
    }

    // Draw trend line
    int n = m_dataPoints.size();
    if (n >= 2) {
        for (int i = 1; i < n; i++) {
            qreal x1 = chartLeft + chartW * (i - 1) / (MAX_POINTS - 1);
            qreal x2 = chartLeft + chartW * i / (MAX_POINTS - 1);
            qreal y1 = chartBottom - chartH * m_dataPoints[i - 1].value / 100.0;
            qreal y2 = chartBottom - chartH * m_dataPoints[i].value / 100.0;

            // Line segment with colour from the data point
            QPen linePen(m_dataPoints[i].color, 2);
            p.setPen(linePen);
            p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }

        // Draw dots at each data point
        for (int i = 0; i < n; i++) {
            qreal x = chartLeft + chartW * i / (MAX_POINTS - 1);
            qreal y = chartBottom - chartH * m_dataPoints[i].value / 100.0;
            p.setPen(Qt::NoPen);
            p.setBrush(m_dataPoints[i].color);
            p.drawEllipse(QPointF(x, y), 3, 3);
        }
    } else if (n == 1) {
        qreal x = chartLeft + chartW * 0 / (MAX_POINTS - 1);
        qreal y = chartBottom - chartH * m_dataPoints[0].value / 100.0;
        p.setPen(Qt::NoPen);
        p.setBrush(m_dataPoints[0].color);
        p.drawEllipse(QPointF(x, y), 4, 4);
    }

    // Draw current value label
    if (n > 0) {
        const DataPoint& last = m_dataPoints.last();
        p.setPen(last.color);
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        QString valText = QString("%1%").arg(qRound(last.value));
        p.drawText(QRectF(chartRight - 60, chartTop, 60, 20),
                   Qt::AlignRight | Qt::AlignTop, valText);
    }
}

// ═══════════════════════════════════════════════════════════════
//  EnlargedComponentView
// ═══════════════════════════════════════════════════════════════

EnlargedComponentView::EnlargedComponentView(const QString& componentId,
                                             ComponentType type,
                                             const QList<SubcomponentInfo>& subcomponents,
                                             QWidget* parent)
    : QWidget(parent)
    , m_componentId(componentId)
    , m_componentType(type)
    , m_currentColor(Qt::blue)
    , m_currentSize(50)
    , m_componentView(nullptr)
    , m_componentScene(nullptr)
    , m_displayComponent(nullptr)
    , m_healthStatusLabel(nullptr)
    , m_healthValueLabel(nullptr)
    , m_trendChart(nullptr)
    , m_analyticsStatusLabel(nullptr)
    , m_updateCountLabel(nullptr)
    , m_statusChangesLabel(nullptr)
    , m_avgHealthLabel(nullptr)
    , m_subcomponents(subcomponents)
    , m_updateCount(0)
    , m_statusChanges(0)
    , m_healthSum(0)
{
    setupUI();
}

EnlargedComponentView::~EnlargedComponentView()
{
}

void EnlargedComponentView::setupUI()
{
    // ── Main horizontal layout: left panel + right panel ──
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ════════════════════════════════════════════════════════
    //  LEFT PANEL – Component enlarged + subsystems
    // ════════════════════════════════════════════════════════
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setObjectName("enlargedLeftPanel");
    leftPanel->setStyleSheet(
        "QWidget#enlargedLeftPanel {"
        "  background: #24272e;"
        "  border: 1px solid #3a3f4b;"
        "  border-radius: 4px;"
        "}");
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(14, 14, 14, 14);

    // Title
    QString typeName = Component::typeName(m_componentType);
    QLabel* leftTitle = new QLabel(typeName.toUpper() + " COMPONENT", leftPanel);
    leftTitle->setStyleSheet(
        "color: #e8eaed; font-size: 15px; font-weight: 700;"
        "letter-spacing: 1.2px; background: transparent; padding: 4px 0;");

    // Health status row
    QWidget* statusRow = new QWidget(leftPanel);
    statusRow->setStyleSheet("background: transparent;");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusRow);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(12);

    m_healthStatusLabel = new QLabel("STATUS: NOMINAL", statusRow);
    m_healthStatusLabel->setStyleSheet(
        "color: #66bb6a; font-size: 11px; font-weight: 600;"
        "padding: 3px 10px; background: rgba(46,125,50,0.15);"
        "border-radius: 3px; border-left: 3px solid #4CAF50;");

    m_healthValueLabel = new QLabel("HEALTH: --", statusRow);
    m_healthValueLabel->setStyleSheet(
        "color: #90caf9; font-size: 11px; font-weight: 600;"
        "padding: 3px 10px; background: rgba(21,101,192,0.15);"
        "border-radius: 3px;");

    statusLayout->addWidget(m_healthStatusLabel);
    statusLayout->addWidget(m_healthValueLabel);
    statusLayout->addStretch();

    // Enlarged component view (QGraphicsView)
    m_componentScene = new QGraphicsScene(this);
    m_componentScene->setSceneRect(0, 0, 400, 280);

    m_displayComponent = new Component(m_componentType, m_componentId + "_enlarged");
    m_displayComponent->setSize(120); // Much larger than normal
    m_displayComponent->setPos(200, 130);
    m_componentScene->addItem(m_displayComponent);

    m_componentView = new QGraphicsView(m_componentScene, leftPanel);
    m_componentView->setRenderHint(QPainter::Antialiasing);
    m_componentView->setBackgroundBrush(QBrush(QColor(24, 26, 31)));
    m_componentView->setMinimumHeight(200);
    m_componentView->setMaximumHeight(300);
    m_componentView->setFrameShape(QFrame::NoFrame);
    m_componentView->setStyleSheet("border: 1px solid #3a3f4b; border-radius: 4px;");

    // Subsystems section
    QLabel* subsysTitle = new QLabel("SUBSYSTEMS", leftPanel);
    subsysTitle->setStyleSheet(
        "color: #9aa0a6; font-size: 11px; font-weight: 600;"
        "letter-spacing: 1px; padding: 6px 0 2px 0; background: transparent;");

    // Subsystem list with scroll area
    QScrollArea* scrollArea = new QScrollArea(leftPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QWidget#subsysContainer { background: transparent; }");

    QWidget* subsysContainer = new QWidget();
    subsysContainer->setObjectName("subsysContainer");
    QVBoxLayout* subsysLayout = new QVBoxLayout(subsysContainer);
    subsysLayout->setSpacing(4);
    subsysLayout->setContentsMargins(0, 0, 0, 0);

    for (const SubcomponentInfo& sub : m_subcomponents) {
        SubsystemHealthBar* bar = new SubsystemHealthBar(sub.name, sub.health, sub.color, subsysContainer);
        subsysLayout->addWidget(bar);
        m_subsystemBars.append(bar);
    }
    subsysLayout->addStretch();
    scrollArea->setWidget(subsysContainer);

    leftLayout->addWidget(leftTitle);
    leftLayout->addWidget(statusRow);
    leftLayout->addWidget(m_componentView, 2);
    leftLayout->addWidget(subsysTitle);
    leftLayout->addWidget(scrollArea, 1);

    // ════════════════════════════════════════════════════════
    //  RIGHT PANEL – Data Analytics
    // ════════════════════════════════════════════════════════
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setObjectName("enlargedRightPanel");
    rightPanel->setStyleSheet(
        "QWidget#enlargedRightPanel {"
        "  background: #24272e;"
        "  border: 1px solid #3a3f4b;"
        "  border-radius: 4px;"
        "}");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setSpacing(12);
    rightLayout->setContentsMargins(14, 14, 14, 14);

    QLabel* rightTitle = new QLabel("DATA ANALYTICS", rightPanel);
    rightTitle->setStyleSheet(
        "color: #e8eaed; font-size: 15px; font-weight: 700;"
        "letter-spacing: 1.2px; background: transparent; padding: 4px 0;");

    // Health trend chart
    QLabel* chartLabel = new QLabel("HEALTH TREND", rightPanel);
    chartLabel->setStyleSheet(
        "color: #9aa0a6; font-size: 10px; font-weight: 600;"
        "letter-spacing: 1px; padding: 2px 0; background: transparent;");

    m_trendChart = new HealthTrendChart(rightPanel);
    m_trendChart->setSubcomponents(m_subcomponents);

    // Stats cards
    QLabel* statsLabel = new QLabel("STATISTICS", rightPanel);
    statsLabel->setStyleSheet(
        "color: #9aa0a6; font-size: 10px; font-weight: 600;"
        "letter-spacing: 1px; padding: 6px 0 2px 0; background: transparent;");

    QWidget* statsContainer = new QWidget(rightPanel);
    statsContainer->setStyleSheet("background: transparent;");
    QGridLayout* statsGrid = new QGridLayout(statsContainer);
    statsGrid->setSpacing(8);
    statsGrid->setContentsMargins(0, 0, 0, 0);

    auto createStatCard = [&](const QString& label, const QString& value, int row, int col) -> QLabel* {
        QWidget* card = new QWidget(statsContainer);
        card->setStyleSheet(
            "background: #1c1e26; border: 1px solid #3a3f4b; border-radius: 4px;");
        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(2);

        QLabel* lbl = new QLabel(label, card);
        lbl->setStyleSheet(
            "color: #6c717a; font-size: 9px; font-weight: 600;"
            "letter-spacing: 0.8px; background: transparent; border: none;");

        QLabel* val = new QLabel(value, card);
        val->setStyleSheet(
            "color: #e8eaed; font-size: 16px; font-weight: 700;"
            "background: transparent; border: none;");

        cardLayout->addWidget(lbl);
        cardLayout->addWidget(val);
        statsGrid->addWidget(card, row, col);
        return val;
    };

    m_updateCountLabel = createStatCard("HEALTH UPDATES", "0", 0, 0);
    m_statusChangesLabel = createStatCard("STATUS CHANGES", "0", 0, 1);
    m_analyticsStatusLabel = createStatCard("CURRENT STATUS", "--", 1, 0);
    m_avgHealthLabel = createStatCard("AVG HEALTH", "--", 1, 1);

    // Subsystem overview section on the analytics side
    QLabel* subOverviewLabel = new QLabel("SUBSYSTEM OVERVIEW", rightPanel);
    subOverviewLabel->setStyleSheet(
        "color: #9aa0a6; font-size: 10px; font-weight: 600;"
        "letter-spacing: 1px; padding: 6px 0 2px 0; background: transparent;");

    QWidget* subOverviewContainer = new QWidget(rightPanel);
    subOverviewContainer->setStyleSheet("background: transparent;");
    QVBoxLayout* subOverviewLayout = new QVBoxLayout(subOverviewContainer);
    subOverviewLayout->setSpacing(3);
    subOverviewLayout->setContentsMargins(0, 0, 0, 0);

    for (const SubcomponentInfo& sub : m_subcomponents) {
        QWidget* row = new QWidget(subOverviewContainer);
        row->setStyleSheet("background: transparent;");
        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(6, 2, 6, 2);
        rowLayout->setSpacing(8);

        // Colour dot
        QLabel* dot = new QLabel(row);
        dot->setFixedSize(8, 8);
        dot->setStyleSheet(
            QString("background: %1; border-radius: 4px; border: none;").arg(sub.color.name()));

        QLabel* name = new QLabel(sub.name, row);
        name->setStyleSheet("color: #c4c7cc; font-size: 10px; background: transparent; border: none;");

        QLabel* pct = new QLabel(QString("%1%").arg(sub.health), row);
        pct->setStyleSheet(
            QString("color: %1; font-size: 10px; font-weight: 700;"
                    "background: transparent; border: none;").arg(sub.color.name()));

        rowLayout->addWidget(dot);
        rowLayout->addWidget(name, 1);
        rowLayout->addWidget(pct);
        subOverviewLayout->addWidget(row);
    }

    rightLayout->addWidget(rightTitle);
    rightLayout->addWidget(chartLabel);
    rightLayout->addWidget(m_trendChart, 2);
    rightLayout->addWidget(statsLabel);
    rightLayout->addWidget(statsContainer);
    rightLayout->addWidget(subOverviewLabel);
    rightLayout->addWidget(subOverviewContainer);
    rightLayout->addStretch();

    // ── Add panels to main layout ──
    mainLayout->addWidget(leftPanel, 3);  // 60%
    mainLayout->addWidget(rightPanel, 2); // 40%
}

void EnlargedComponentView::updateComponentHealth(const QColor& color, qreal size)
{
    m_currentColor = color;
    m_currentSize = size;
    m_updateCount++;
    m_healthSum += size;

    // Update enlarged component visual
    if (m_displayComponent) {
        m_displayComponent->setColor(color);
        // Keep the enlarged size stable but update colour
    }

    // Update status labels
    QString statusText = healthStatusText(color);
    m_healthStatusLabel->setText("STATUS: " + statusText);

    // Colour-code the status label
    if (color.name() == "#4caf50" || color.name() == "#4CAF50" || color == QColor(Qt::green) ||
        (color.green() > 150 && color.red() < 150)) {
        m_healthStatusLabel->setStyleSheet(
            "color: #66bb6a; font-size: 11px; font-weight: 600;"
            "padding: 3px 10px; background: rgba(46,125,50,0.15);"
            "border-radius: 3px; border-left: 3px solid #4CAF50;");
    } else if (color.name() == "#f44336" || color.name() == "#F44336" || (color.red() > 200 && color.green() < 100)) {
        m_healthStatusLabel->setStyleSheet(
            "color: #ef5350; font-size: 11px; font-weight: 600;"
            "padding: 3px 10px; background: rgba(183,28,28,0.15);"
            "border-radius: 3px; border-left: 3px solid #f44336;");
    } else {
        m_healthStatusLabel->setStyleSheet(
            "color: #ffb74d; font-size: 11px; font-weight: 600;"
            "padding: 3px 10px; background: rgba(230,126,34,0.15);"
            "border-radius: 3px; border-left: 3px solid #FF9800;");
    }

    m_healthValueLabel->setText(QString("HEALTH: %1%").arg(qRound(size)));

    // Track status changes
    if (!m_lastColor.isEmpty() && m_lastColor != color.name()) {
        m_statusChanges++;
    }
    m_lastColor = color.name();

    // Update analytics
    m_trendChart->addDataPoint(size, color);
    m_updateCountLabel->setText(QString::number(m_updateCount));
    m_statusChangesLabel->setText(QString::number(m_statusChanges));
    m_analyticsStatusLabel->setText(statusText);

    qreal avg = m_healthSum / m_updateCount;
    m_avgHealthLabel->setText(QString("%1%").arg(qRound(avg)));

    // Colour the analytics status label
    m_analyticsStatusLabel->setStyleSheet(
        QString("color: %1; font-size: 16px; font-weight: 700;"
                "background: transparent; border: none;").arg(color.name()));
}

QString EnlargedComponentView::healthStatusText(const QColor& color) const
{
    // Map colour to health status
    QString name = color.name().toLower();
    if (name == "#4caf50" || name == "#66bb6a" || (color.green() > 150 && color.red() < 120))
        return "NOMINAL";
    if (name == "#ffc107" || name == "#ffb74d" || (color.red() > 200 && color.green() > 150))
        return "WARNING";
    if (name == "#ff9800" || name == "#e65100")
        return "DEGRADED";
    if (name == "#f44336" || name == "#d32f2f" || (color.red() > 200 && color.green() < 80))
        return "CRITICAL";
    if (name == "#03a9f4" || name == "#2196f3")
        return "NOMINAL";
    if (name == "#9c27b0")
        return "NOMINAL";
    return "ACTIVE";
}
