#include "connection.h"
#include "component.h"
#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <QFontMetrics>

Connection::Connection(Component* source, Component* target, 
                       ConnectionType type, const QString& label,
                       QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_source(source)
    , m_target(target)
    , m_connectionType(type)
    , m_label(label)
    , m_color(QColor(100, 180, 220))
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setZValue(-1);  // Draw behind components
    
    updatePosition();
}

void Connection::updatePosition()
{
    if (!m_source || !m_target) return;
    
    prepareGeometryChange();
    
    QPointF srcCenter = m_source->sceneBoundingRect().center();
    QPointF tgtCenter = m_target->sceneBoundingRect().center();
    
    m_sourcePoint = getConnectionPoint(m_source, tgtCenter);
    m_targetPoint = getConnectionPoint(m_target, srcCenter);
}

QPointF Connection::getConnectionPoint(Component* comp, const QPointF& otherCenter) const
{
    QRectF rect = comp->sceneBoundingRect();
    QPointF center = rect.center();
    
    // Calculate intersection of line from center to otherCenter with the component rect
    qreal dx = otherCenter.x() - center.x();
    qreal dy = otherCenter.y() - center.y();
    
    if (qAbs(dx) < 0.001 && qAbs(dy) < 0.001) {
        return center;
    }
    
    qreal halfW = rect.width() / 2.0;
    qreal halfH = rect.height() / 2.0;
    
    // Use parametric intersection with rectangle edges
    qreal tX = (dx != 0) ? halfW / qAbs(dx) : 1e9;
    qreal tY = (dy != 0) ? halfH / qAbs(dy) : 1e9;
    qreal t = qMin(tX, tY);
    
    return QPointF(center.x() + dx * t, center.y() + dy * t);
}

QRectF Connection::boundingRect() const
{
    if (!m_source || !m_target) return QRectF();
    
    qreal extra = 30;  // Extra space for arrowheads and labels
    QRectF rect = QRectF(m_sourcePoint, m_targetPoint).normalized();
    return rect.adjusted(-extra, -extra, extra, extra);
}

QPainterPath Connection::shape() const
{
    QPainterPath path;
    if (!m_source || !m_target) return path;
    
    // Create a thick line shape for easier selection
    QLineF line(m_sourcePoint, m_targetPoint);
    qreal angle = line.angle();
    
    QPointF offset(6 * qSin(qDegreesToRadians(angle)), 
                   6 * qCos(qDegreesToRadians(angle)));
    
    QPolygonF polygon;
    polygon << m_sourcePoint + offset 
            << m_targetPoint + offset 
            << m_targetPoint - offset 
            << m_sourcePoint - offset;
    
    path.addPolygon(polygon);
    path.closeSubpath();
    return path;
}

void Connection::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    if (!m_source || !m_target) return;
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    updatePosition();
    
    QColor lineColor = isSelected() ? QColor("#00BCD4") : m_color;
    qreal lineWidth = isSelected() ? 2.5 : 1.8;
    
    // Draw the main line
    QPen linePen(lineColor, lineWidth);
    linePen.setCapStyle(Qt::RoundCap);
    painter->setPen(linePen);
    painter->drawLine(m_sourcePoint, m_targetPoint);
    
    // Draw arrowhead(s)
    qreal arrowSize = 10.0;
    
    // Always draw arrow at target
    drawArrowHead(painter, m_targetPoint, m_sourcePoint, arrowSize);
    
    // If bidirectional, draw arrow at source too
    if (m_connectionType == ConnectionType::Bidirectional) {
        drawArrowHead(painter, m_sourcePoint, m_targetPoint, arrowSize);
    }
    
    // Draw label text
    if (!m_label.isEmpty()) {
        QPointF midPoint = (m_sourcePoint + m_targetPoint) / 2.0;
        
        // Calculate angle of line
        QLineF line(m_sourcePoint, m_targetPoint);
        qreal angle = -line.angle();
        
        // Make text always readable (not upside down)
        if (angle < -90 || angle > 90) {
            angle += 180;
        }
        
        painter->save();
        painter->translate(midPoint);
        painter->rotate(angle);
        
        // Draw label background
        QFont labelFont("Segoe UI", 8, QFont::Bold);
        painter->setFont(labelFont);
        QFontMetrics fm(labelFont);
        QRectF textRect = fm.boundingRect(m_label);
        textRect.adjust(-6, -3, 6, 3);
        textRect.moveCenter(QPointF(0, -10));
        
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(24, 26, 31, 220));
        painter->drawRoundedRect(textRect, 4, 4);
        
        // Draw border
        painter->setPen(QPen(lineColor.darker(120), 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(textRect, 4, 4);
        
        // Draw text
        painter->setPen(lineColor.lighter(130));
        painter->drawText(textRect, Qt::AlignCenter, m_label);
        
        painter->restore();
    }
    
    // Draw selection indicators
    if (isSelected()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#00BCD4"));
        painter->drawEllipse(m_sourcePoint, 4, 4);
        painter->drawEllipse(m_targetPoint, 4, 4);
    }
}

void Connection::drawArrowHead(QPainter* painter, const QPointF& tip, const QPointF& from, qreal size)
{
    QLineF line(from, tip);
    qreal angle = qAtan2(-line.dy(), line.dx());
    
    QPointF arrowP1 = tip - QPointF(size * qCos(angle - M_PI / 7),
                                     -size * qSin(angle - M_PI / 7));
    QPointF arrowP2 = tip - QPointF(size * qCos(angle + M_PI / 7),
                                     -size * qSin(angle + M_PI / 7));
    
    QColor arrowColor = isSelected() ? QColor("#00BCD4") : m_color;
    
    painter->setPen(Qt::NoPen);
    painter->setBrush(arrowColor);
    
    QPolygonF arrowHead;
    arrowHead << tip << arrowP1 << arrowP2;
    painter->drawPolygon(arrowHead);
}

void Connection::setLabel(const QString& label)
{
    m_label = label;
    update();
}

void Connection::setConnectionType(ConnectionType type)
{
    m_connectionType = type;
    update();
}

void Connection::setColor(const QColor& color)
{
    m_color = color;
    update();
}

bool Connection::involvesComponent(const Component* comp) const
{
    return (m_source == comp || m_target == comp);
}

QString Connection::connectionTypeToString(ConnectionType type)
{
    switch (type) {
        case ConnectionType::Unidirectional: return "unidirectional";
        case ConnectionType::Bidirectional: return "bidirectional";
        default: return "unidirectional";
    }
}

ConnectionType Connection::stringToConnectionType(const QString& str)
{
    if (str == "bidirectional") return ConnectionType::Bidirectional;
    return ConnectionType::Unidirectional;
}
