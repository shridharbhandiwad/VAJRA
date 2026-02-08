#include "component.h"
#include <QPainter>
#include <QCursor>

Component::Component(ComponentType type, const QString& id, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_type(type)
    , m_id(id)
    , m_color(Qt::blue)
    , m_size(50)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setCursor(Qt::OpenHandCursor);
}

QRectF Component::boundingRect() const
{
    qreal halfSize = m_size / 2.0;
    return QRectF(-halfSize, -halfSize, m_size, m_size);
}

void Component::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(m_color);
    
    qreal halfSize = m_size / 2.0;
    
    switch (m_type) {
        case ComponentType::Circle:
            painter->drawEllipse(-halfSize, -halfSize, m_size, m_size);
            break;
            
        case ComponentType::Square:
            painter->drawRect(-halfSize, -halfSize, m_size, m_size);
            break;
            
        case ComponentType::Triangle: {
            QPointF points[3] = {
                QPointF(0, -halfSize),
                QPointF(-halfSize, halfSize),
                QPointF(halfSize, halfSize)
            };
            painter->drawPolygon(points, 3);
            break;
        }
    }
    
    // Draw selection border if selected
    if (isSelected()) {
        painter->setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
}

void Component::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void Component::setSize(qreal size)
{
    prepareGeometryChange();
    m_size = size;
    update();
}

QString Component::toJson() const
{
    QString typeStr;
    switch (m_type) {
        case ComponentType::Circle: typeStr = "Circle"; break;
        case ComponentType::Square: typeStr = "Square"; break;
        case ComponentType::Triangle: typeStr = "Triangle"; break;
    }
    
    return QString("{\"id\":\"%1\",\"type\":\"%2\",\"x\":%3,\"y\":%4,\"color\":\"%5\",\"size\":%6}")
        .arg(m_id)
        .arg(typeStr)
        .arg(pos().x())
        .arg(pos().y())
        .arg(m_color.name())
        .arg(m_size);
}

Component* Component::fromJson(const QString& id, ComponentType type, qreal x, qreal y, 
                                const QColor& color, qreal size)
{
    Component* comp = new Component(type, id);
    comp->setPos(x, y);
    comp->setColor(color);
    comp->setSize(size);
    return comp;
}

QVariant Component::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene()) {
        // Component position changed
    }
    return QGraphicsItem::itemChange(change, value);
}
