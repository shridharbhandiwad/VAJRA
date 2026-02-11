#include "connection.h"
#include "component.h"
#include "subcomponent.h"
#include "thememanager.h"
#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <QFontMetrics>

Connection::Connection(Component* source, Component* target, 
                       ConnectionType type, const QString& label,
                       QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_sourceItem(source)
    , m_targetItem(target)
    , m_connectionType(type)
    , m_label(label)
    , m_color(ThemeManager::instance().connectionDefaultColor())
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setZValue(-1);  // Draw behind components
    
    updatePosition();
}

Connection::Connection(QGraphicsItem* source, QGraphicsItem* target,
                       ConnectionType type, const QString& label,
                       QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_sourceItem(source)
    , m_targetItem(target)
    , m_connectionType(type)
    , m_label(label)
    , m_color(ThemeManager::instance().connectionDefaultColor())
{
    setFlag(QGraphicsItem::ItemIsSelectable);
    setZValue(-1);  // Draw behind components
    
    updatePosition();
}

Component* Connection::getSource() const
{
    return qgraphicsitem_cast<Component*>(m_sourceItem);
}

Component* Connection::getTarget() const
{
    return qgraphicsitem_cast<Component*>(m_targetItem);
}

SubComponent* Connection::getSourceSub() const
{
    return qgraphicsitem_cast<SubComponent*>(m_sourceItem);
}

SubComponent* Connection::getTargetSub() const
{
    return qgraphicsitem_cast<SubComponent*>(m_targetItem);
}

QPointF Connection::getItemCenter(QGraphicsItem* item) const
{
    if (!item) return QPointF();
    
    // Check if it's a SubComponent - use its centerInScene method
    SubComponent* sub = qgraphicsitem_cast<SubComponent*>(item);
    if (sub) {
        return sub->centerInScene();
    }
    
    // Otherwise use the bounding rect center
    return item->sceneBoundingRect().center();
}

void Connection::updatePosition()
{
    if (!m_sourceItem || !m_targetItem) return;
    
    prepareGeometryChange();
    
    QPointF srcCenter = getItemCenter(m_sourceItem);
    QPointF tgtCenter = getItemCenter(m_targetItem);
    
    m_sourcePoint = getConnectionPoint(m_sourceItem, tgtCenter);
    m_targetPoint = getConnectionPoint(m_targetItem, srcCenter);
}

QPointF Connection::getConnectionPoint(QGraphicsItem* item, const QPointF& otherCenter) const
{
    if (!item) return QPointF();
    
    QRectF rect = item->sceneBoundingRect();
    QPointF center = rect.center();
    
    // Calculate intersection of line from center to otherCenter with the item rect
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
    if (!m_sourceItem || !m_targetItem) return QRectF();
    
    qreal extra = 30;  // Extra space for arrowheads and labels
    QRectF rect = QRectF(m_sourcePoint, m_targetPoint).normalized();
    return rect.adjusted(-extra, -extra, extra, extra);
}

QPainterPath Connection::shape() const
{
    QPainterPath path;
    if (!m_sourceItem || !m_targetItem) return path;
    
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
    
    if (!m_sourceItem || !m_targetItem) return;
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    updatePosition();
    
    ThemeManager& tm = ThemeManager::instance();
    QColor lineColor = isSelected() ? tm.accentPrimary() : m_color;
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
        QFont labelFont("Inter", 10, QFont::Bold);
        painter->setFont(labelFont);
        QFontMetrics fm(labelFont);
        QRectF textRect = fm.boundingRect(m_label);
        textRect.adjust(-6, -3, 6, 3);
        textRect.moveCenter(QPointF(0, -10));
        
        painter->setPen(Qt::NoPen);
        painter->setBrush(ThemeManager::instance().connectionLabelBackground());
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
        ThemeManager& tm = ThemeManager::instance();
        painter->setPen(Qt::NoPen);
        painter->setBrush(tm.accentPrimary());
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
    
    ThemeManager& tm = ThemeManager::instance();
    QColor arrowColor = isSelected() ? tm.accentPrimary() : m_color;
    
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
    Component* src = qgraphicsitem_cast<Component*>(m_sourceItem);
    Component* tgt = qgraphicsitem_cast<Component*>(m_targetItem);
    return (src == comp || tgt == comp);
}

bool Connection::involvesSubComponent(const SubComponent* sub) const
{
    SubComponent* src = qgraphicsitem_cast<SubComponent*>(m_sourceItem);
    SubComponent* tgt = qgraphicsitem_cast<SubComponent*>(m_targetItem);
    return (src == sub || tgt == sub);
}

bool Connection::involvesItem(const QGraphicsItem* item) const
{
    return (m_sourceItem == item || m_targetItem == item);
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
