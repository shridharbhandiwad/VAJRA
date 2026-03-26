#include "trmgridview.h"
#include "thememanager.h"
#include <QPainterPath>
#include <QFrame>
#include <QDebug>
#include <cmath>

// ════════════════════════════════════════════════════════════════════
//  TrmCell
// ════════════════════════════════════════════════════════════════════

TrmCell::TrmCell(int trmId, QWidget* parent)
    : QWidget(parent)
    , m_id(trmId)
{
    m_data.id = trmId;
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
    setToolTip(QString("TRM %1").arg(trmId));
}

void TrmCell::setData(const TrmData& data)
{
    m_data = data;
    // Build tooltip
    QString tip = QString(
        "<b>TRM %1</b><br>"
        "Health: %2%<br>"
        "Status: %3<br>"
        "Voltage: %4 V<br>"
        "Current: %5 A<br>"
        "On: %6<br>"
        "Trip: %7<br>"
        "BIT: %8"
    ).arg(m_id)
     .arg(QString::number(data.health, 'f', 1))
     .arg(data.status)
     .arg(QString::number(data.voltage, 'f', 2))
     .arg(QString::number(data.current, 'f', 2))
     .arg(data.on ? "Yes" : "No")
     .arg(data.trip)
     .arg(data.bit);
    if (data.quadrant >= 0)
        tip += QString("<br>Quadrant: %1  Ch: %2").arg(data.quadrant).arg(data.channel);
    setToolTip(tip);
    update();
}

QSize TrmCell::sizeHint() const { return QSize(32, 22); }
QSize TrmCell::minimumSizeHint() const { return QSize(24, 16); }

void TrmCell::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor cellColor(m_data.color.isEmpty() ? "#808080" : m_data.color);

    // Border highlight on hover / selection
    if (m_hovered) {
        p.setPen(QPen(Qt::white, 2));
    } else {
        p.setPen(QPen(cellColor.darker(160), 1));
    }

    // Fill
    p.setBrush(cellColor);
    p.drawRect(1, 1, width() - 2, height() - 2);

    // ID label
    // Choose contrasting text colour
    int luminance = (cellColor.red() * 299 + cellColor.green() * 587 + cellColor.blue() * 114) / 1000;
    QColor textColor = (luminance > 128) ? QColor(0, 0, 0, 200) : QColor(255, 255, 255, 220);

    p.setPen(textColor);
    QFont f;
    f.setPixelSize(qMax(8, height() - 8));
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect(), Qt::AlignCenter, QString::number(m_id));
}

void TrmCell::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked(m_id);
    QWidget::mousePressEvent(event);
}

void TrmCell::enterEvent(QEvent* event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void TrmCell::leaveEvent(QEvent* event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

// ════════════════════════════════════════════════════════════════════
//  TrmLegend
// ════════════════════════════════════════════════════════════════════

TrmLegend::TrmLegend(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(22);
}

QSize TrmLegend::sizeHint() const { return QSize(300, 22); }

void TrmLegend::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    struct Entry { QString label; QColor color; };
    static const Entry entries[] = {
        {"HEALTHY",  QColor("#00FF00")},
        {"WARNING",  QColor("#FFFF00")},
        {"DEGRADED", QColor("#FFA500")},
        {"CRITICAL", QColor("#FF0000")},
        {"OFFLINE",  QColor("#808080")},
    };

    int n = sizeof(entries) / sizeof(entries[0]);
    int swatchW = 14, swatchH = 12;
    int spacing = 6;
    int textW = 60;
    int totalW = n * (swatchW + spacing + textW + spacing);
    int startX = (width() - totalW) / 2;
    int y = (height() - swatchH) / 2;

    ThemeManager& tm = ThemeManager::instance();
    p.setPen(tm.primaryText());
    p.setFont(QFont("Inter", 9));

    for (int i = 0; i < n; ++i) {
        int x = startX + i * (swatchW + spacing + textW + spacing);
        // Swatch
        p.setBrush(entries[i].color);
        p.setPen(QPen(entries[i].color.darker(150), 1));
        p.drawRect(x, y, swatchW, swatchH);
        // Label
        p.setPen(tm.primaryText());
        p.drawText(x + swatchW + 3, 0, textW, height(),
                   Qt::AlignLeft | Qt::AlignVCenter, entries[i].label);
    }
}

// ════════════════════════════════════════════════════════════════════
//  TrmStatusBar
// ════════════════════════════════════════════════════════════════════

TrmStatusBar::TrmStatusBar(QWidget* parent) : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(28);
}

QSize TrmStatusBar::sizeHint() const { return QSize(400, 28); }

void TrmStatusBar::update(int healthy, int warning, int critical, int unknown, int total)
{
    m_healthy = healthy;
    m_warning = warning;
    m_critical = critical;
    m_unknown = unknown;
    m_total = total;
    QWidget::update();
}

void TrmStatusBar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    ThemeManager& tm = ThemeManager::instance();

    struct Stat { QString label; int value; QColor color; };
    Stat stats[] = {
        {"HLTH",  m_healthy,  QColor("#00CC00")},
        {"WARN",  m_warning,  QColor("#CCCC00")},
        {"CRIT",  m_critical, QColor("#CC0000")},
        {"UNKN",  m_unknown,  QColor("#808080")},
    };

    int n = 4;
    int cellW = width() / n;

    for (int i = 0; i < n; ++i) {
        int x = i * cellW;
        QRect r(x + 2, 2, cellW - 4, height() - 4);

        // Background pill
        p.setPen(QPen(stats[i].color.darker(140), 1));
        p.setBrush(QColor(stats[i].color.red(), stats[i].color.green(),
                          stats[i].color.blue(), 40));
        p.drawRoundedRect(r, 4, 4);

        // Text
        p.setPen(stats[i].color);
        p.setFont(QFont("Inter", 9, QFont::Bold));
        p.drawText(r, Qt::AlignCenter,
                   QString("%1 %2").arg(stats[i].label).arg(stats[i].value));
    }

    // Total on the right side
    p.setPen(tm.primaryText());
    p.setFont(QFont("Inter", 8));
    p.drawText(width() - 70, 0, 68, height(), Qt::AlignRight | Qt::AlignVCenter,
               QString("TOTAL: %1").arg(m_total));
}

// ════════════════════════════════════════════════════════════════════
//  TrmGridView
// ════════════════════════════════════════════════════════════════════

TrmGridView::TrmGridView(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // Title
    m_titleLabel = new QLabel("ANTENNA SUBSYSTEM – TRM HEALTH GRID", this);
    m_titleLabel->setObjectName("trmGridTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont("Inter", 11, QFont::Bold);
    m_titleLabel->setFont(titleFont);

    // Status bar
    m_statusBar = new TrmStatusBar(this);

    // Scroll area for the grid
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* gridContainer = new QWidget();
    gridContainer->setObjectName("trmGridContainer");
    m_gridLayout = new QGridLayout(gridContainer);
    m_gridLayout->setSpacing(2);
    m_gridLayout->setContentsMargins(4, 4, 4, 4);
    m_scrollArea->setWidget(gridContainer);

    // Legend
    m_legend = new TrmLegend(this);

    // Detail panel
    m_detailLabel = new QLabel("Click a TRM cell for details", this);
    m_detailLabel->setObjectName("trmDetailLabel");
    m_detailLabel->setAlignment(Qt::AlignCenter);
    m_detailLabel->setWordWrap(true);
    m_detailLabel->setMaximumHeight(52);
    m_detailLabel->setStyleSheet("padding: 4px; font-size: 11px;");

    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_statusBar);
    mainLayout->addWidget(m_scrollArea, 1);
    mainLayout->addWidget(m_legend);
    mainLayout->addWidget(m_detailLabel);
}

TrmGridView::~TrmGridView() {}

void TrmGridView::setTrmCount(int count, int columns)
{
    if (count == m_trmCount && columns == m_columns) return;
    m_trmCount = count;
    m_columns = qMax(1, columns);
    m_cells.clear();

    // Clear existing grid items
    while (m_gridLayout->count() > 0) {
        QLayoutItem* item = m_gridLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    buildGrid();
    m_titleLabel->setText(
        QString("ANTENNA SUBSYSTEM – %1 TRM HEALTH GRID  (%2 cols)")
        .arg(count).arg(columns));
    updateStatusCounts();
}

void TrmGridView::buildGrid()
{
    m_cells.resize(m_trmCount);

    for (int i = 0; i < m_trmCount; ++i) {
        TrmCell* cell = new TrmCell(i, m_scrollArea->widget());
        connect(cell, &TrmCell::clicked, this, &TrmGridView::onCellClicked);
        m_cells[i] = cell;
        int row = i / m_columns;
        int col = i % m_columns;
        m_gridLayout->addWidget(cell, row, col);
    }

    // Add row/column header labels
    // Column headers (channel numbers)
    for (int c = 0; c < m_columns; ++c) {
        QLabel* hdr = new QLabel(QString::number(c), m_scrollArea->widget());
        hdr->setAlignment(Qt::AlignCenter);
        QFont f; f.setPixelSize(9); hdr->setFont(f);
        hdr->setStyleSheet("color: #888888;");
        m_gridLayout->addWidget(hdr, m_trmCount / m_columns + 1, c);
    }
}

void TrmGridView::reset()
{
    for (TrmCell* cell : m_cells) {
        TrmCell::TrmData d;
        d.id = cell->trmId();
        d.health = 0;
        d.color = "#808080";
        d.status = "UNKNOWN";
        cell->setData(d);
    }
    updateStatusCounts();
}

void TrmGridView::updateTrmData(const QJsonArray& trmArray)
{
    for (const QJsonValue& v : trmArray) {
        QJsonObject obj = v.toObject();
        int id = obj["id"].toInt(-1);
        if (id < 0 || id >= m_cells.size()) continue;

        TrmCell::TrmData d;
        d.id = id;
        d.health = obj["health"].toDouble(100.0);
        d.color = obj["color"].toString("#00FF00");
        d.status = obj["status"].toString("HEALTHY");
        d.voltage = obj["voltage"].toDouble(0.0);
        d.current = obj["current"].toDouble(0.0);
        d.on = obj["on"].toBool(true);
        d.trip = obj["trip"].toString("Normal");
        d.bit = obj["bit"].toString("Normal");
        d.quadrant = obj["quadrant"].toInt(-1);
        d.channel = obj["channel"].toInt(-1);

        m_cells[id]->setData(d);
    }
    updateStatusCounts();
}

void TrmGridView::updateTrm(int id, const TrmCell::TrmData& data)
{
    if (id < 0 || id >= m_cells.size()) return;
    m_cells[id]->setData(data);
    updateStatusCounts();
}

void TrmGridView::updateStatusCounts()
{
    int healthy = 0, warning = 0, critical = 0, unknown = 0;
    for (TrmCell* cell : m_cells) {
        const TrmCell::TrmData& d = cell->data();
        if (d.color == "#808080" || d.status == "UNKNOWN") { unknown++; continue; }
        if (d.health >= 90)      healthy++;
        else if (d.health >= 70) warning++;
        else if (d.health >= 10) critical++;
        else                     unknown++;
    }
    m_statusBar->update(healthy, warning, critical, unknown, m_cells.size());
}

void TrmGridView::onCellClicked(int trmId)
{
    if (trmId < 0 || trmId >= m_cells.size()) return;
    const TrmCell::TrmData& d = m_cells[trmId]->data();

    QString detail = QString(
        "TRM %1  |  Health: %2%  |  Status: %3  |  V: %4V  |  I: %5A"
    ).arg(trmId)
     .arg(QString::number(d.health, 'f', 1))
     .arg(d.status)
     .arg(QString::number(d.voltage, 'f', 2))
     .arg(QString::number(d.current, 'f', 2));

    if (d.quadrant >= 0)
        detail += QString("  |  Q%1 Ch%2").arg(d.quadrant).arg(d.channel);

    m_detailLabel->setText(detail);
    m_detailLabel->setStyleSheet(
        QString("padding: 4px; font-size: 11px; font-weight: 600; color: %1;")
        .arg(d.color));

    emit trmSelected(trmId, d);
}
