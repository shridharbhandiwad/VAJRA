#include "subcomponent.h"
#include "component.h"
#include "thememanager.h"
#include <QPainter>
#include <QDebug>
#include <QCursor>

constexpr qreal SubComponent::RESIZE_HANDLE_SIZE;

SubComponent::SubComponent(const QString& name, int index, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_name(name)
    , m_color(QColor("#4CAF50"))  // Default green (healthy)
    , m_health(100.0)
    , m_index(index)
    , m_width(defaultWidth())
    , m_height(defaultHeight())
    , m_activeHandle(HandleNone)
    , m_resizing(false)
    , m_dragging(false)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
}

QRectF SubComponent::boundingRect() const
{
    // Add extra space for resize handles
    qreal margin = RESIZE_HANDLE_SIZE;
    return QRectF(-margin, -margin, m_width + margin * 2, m_height + margin * 2);
}

void SubComponent::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    ThemeManager& tm = ThemeManager::instance();
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    qreal w = m_width;
    qreal h = m_height;
    
    // Background with rounded corners
    painter->setPen(QPen(tm.subcomponentBorder(), 1));
    painter->setBrush(tm.subcomponentBackground());
    painter->drawRoundedRect(0, 0, w, h, 4, 4);
    
    // Health indicator bar on the left
    qreal barWidth = 4;
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawRoundedRect(0, 0, barWidth, h, 2, 2);
    
    // Sub-component name
    painter->setPen(tm.subcomponentText());
    painter->setFont(QFont("Inter", 9, QFont::Normal));
    QRectF textRect(barWidth + 6, 0, w - barWidth - 40, h);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_name);
    
    // Health percentage on the right (if there's enough space)
    if (w > 80) {
        painter->setPen(m_color);
        painter->setFont(QFont("Inter", 9, QFont::Bold));
        QRectF healthRect(w - 38, 0, 34, h);
        painter->drawText(healthRect, Qt::AlignVCenter | Qt::AlignRight, 
                          QString("%1%").arg(qRound(m_health)));
    }
    
    // Selection highlight and resize handles
    if (isSelected()) {
        paintResizeHandles(painter);
    }
}

void SubComponent::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void SubComponent::setHealth(qreal health)
{
    m_health = qBound(0.0, health, 100.0);
    update();
}

void SubComponent::setIndex(int index)
{
    m_index = index;
}

void SubComponent::setWidth(qreal w)
{
    prepareGeometryChange();
    m_width = qMax(minWidth(), w);
    update();
}

void SubComponent::setHeight(qreal h)
{
    prepareGeometryChange();
    m_height = qMax(minHeight(), h);
    update();
}

QPointF SubComponent::centerInParent() const
{
    return pos() + QPointF(m_width / 2.0, m_height / 2.0);
}

QPointF SubComponent::centerInScene() const
{
    return sceneBoundingRect().center();
}

Component* SubComponent::parentComponent() const
{
    return qgraphicsitem_cast<Component*>(parentItem());
}

// ======================================================================
// Resize handle hit-testing
// ======================================================================

SubComponent::ResizeHandle SubComponent::handleAt(const QPointF& pos) const
{
    if (!isSelected()) return HandleNone;
    
    qreal w = m_width;
    qreal h = m_height;
    qreal hs = RESIZE_HANDLE_SIZE * 2.0; // hit-test tolerance
    
    // Corner handles
    QRectF tl(-hs / 2, -hs / 2, hs, hs);
    QRectF tr(w - hs / 2, -hs / 2, hs, hs);
    QRectF bl(-hs / 2, h - hs / 2, hs, hs);
    QRectF br(w - hs / 2, h - hs / 2, hs, hs);
    
    if (tl.contains(pos)) return HandleTopLeft;
    if (tr.contains(pos)) return HandleTopRight;
    if (bl.contains(pos)) return HandleBottomLeft;
    if (br.contains(pos)) return HandleBottomRight;
    
    // Edge handles
    QRectF top(hs / 2, -hs / 2, w - hs, hs);
    QRectF bottom(hs / 2, h - hs / 2, w - hs, hs);
    QRectF left(-hs / 2, hs / 2, hs, h - hs);
    QRectF right(w - hs / 2, hs / 2, hs, h - hs);
    
    if (top.contains(pos)) return HandleTop;
    if (bottom.contains(pos)) return HandleBottom;
    if (left.contains(pos)) return HandleLeft;
    if (right.contains(pos)) return HandleRight;
    
    return HandleNone;
}

void SubComponent::paintResizeHandles(QPainter* painter)
{
    qreal w = m_width;
    qreal h = m_height;
    qreal hs = RESIZE_HANDLE_SIZE;
    
    // Selection dashed border
    painter->setPen(QPen(QColor("#00BCD4"), 1.5, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(QRectF(0, 0, w, h), 4, 4);
    
    // Draw handle squares at corners and edges
    painter->setPen(QPen(Qt::white, 0.5));
    painter->setBrush(QColor("#00BCD4"));
    
    QRectF handles[] = {
        QRectF(-hs / 2, -hs / 2, hs, hs),                          // TopLeft
        QRectF(w / 2 - hs / 2, -hs / 2, hs, hs),                   // Top
        QRectF(w - hs / 2, -hs / 2, hs, hs),                       // TopRight
        QRectF(w - hs / 2, h / 2 - hs / 2, hs, hs),                // Right
        QRectF(w - hs / 2, h - hs / 2, hs, hs),                    // BottomRight
        QRectF(w / 2 - hs / 2, h - hs / 2, hs, hs),                // Bottom
        QRectF(-hs / 2, h - hs / 2, hs, hs),                       // BottomLeft
        QRectF(-hs / 2, h / 2 - hs / 2, hs, hs),                   // Left
    };
    
    for (const auto& rect : handles) {
        painter->drawRect(rect);
    }
}

// ======================================================================
// Position constraint to stay within parent boundary
// ======================================================================

QPointF SubComponent::constrainToBoundary(const QPointF& proposedPos) const
{
    Component* parent = parentComponent();
    if (!parent) return proposedPos;
    
    // Get the design container area where subcomponents should stay
    QRectF containerRect = parent->designContainerRect();
    
    // Constrain position so the subcomponent stays within the design container
    qreal x = proposedPos.x();
    qreal y = proposedPos.y();
    
    // Ensure left edge is within bounds
    x = qMax(containerRect.left(), x);
    // Ensure right edge is within bounds
    x = qMin(containerRect.right() - m_width, x);
    
    // Ensure top edge is within bounds
    y = qMax(containerRect.top(), y);
    // Ensure bottom edge is within bounds
    y = qMin(containerRect.bottom() - m_height, y);
    
    return QPointF(x, y);
}

// ======================================================================
// Mouse events for dragging and resizing
// ======================================================================

void SubComponent::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && isSelected()) {
        ResizeHandle handle = handleAt(event->pos());
        if (handle != HandleNone) {
            m_activeHandle = handle;
            m_resizing = true;
            m_lastMouseScenePos = event->scenePos();
            event->accept();
            return;
        }
    }
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsItem::mousePressEvent(event);
}

void SubComponent::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_resizing && m_activeHandle != HandleNone) {
        QPointF delta = event->scenePos() - m_lastMouseScenePos;
        m_lastMouseScenePos = event->scenePos();
        
        prepareGeometryChange();
        
        qreal curW = m_width;
        qreal curH = m_height;
        QPointF curPos = pos();
        qreal newX = curPos.x();
        qreal newY = curPos.y();
        qreal newW = curW;
        qreal newH = curH;
        
        switch (m_activeHandle) {
        case HandleTopLeft:
            newX += delta.x();
            newY += delta.y();
            newW -= delta.x();
            newH -= delta.y();
            break;
        case HandleTop:
            newY += delta.y();
            newH -= delta.y();
            break;
        case HandleTopRight:
            newY += delta.y();
            newW += delta.x();
            newH -= delta.y();
            break;
        case HandleRight:
            newW += delta.x();
            break;
        case HandleBottomRight:
            newW += delta.x();
            newH += delta.y();
            break;
        case HandleBottom:
            newH += delta.y();
            break;
        case HandleBottomLeft:
            newX += delta.x();
            newW -= delta.x();
            newH += delta.y();
            break;
        case HandleLeft:
            newX += delta.x();
            newW -= delta.x();
            break;
        default:
            break;
        }
        
        // Enforce minimum sizes
        if (newW < minWidth()) {
            if (m_activeHandle == HandleTopLeft || m_activeHandle == HandleBottomLeft || m_activeHandle == HandleLeft)
                newX = curPos.x() + curW - minWidth();
            newW = minWidth();
        }
        if (newH < minHeight()) {
            if (m_activeHandle == HandleTopLeft || m_activeHandle == HandleTopRight || m_activeHandle == HandleTop)
                newY = curPos.y() + curH - minHeight();
            newH = minHeight();
        }
        
        // Constrain to parent boundary
        QPointF constrainedPos = constrainToBoundary(QPointF(newX, newY));
        setPos(constrainedPos);
        m_width = newW;
        m_height = newH;
        
        update();
        event->accept();
        return;
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void SubComponent::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_resizing) {
        m_resizing = false;
        m_activeHandle = HandleNone;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    if (m_dragging) {
        m_dragging = false;
    }
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::mouseReleaseEvent(event);
}

// ======================================================================
// Hover events for resize cursor feedback
// ======================================================================

void SubComponent::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (isSelected()) {
        ResizeHandle handle = handleAt(event->pos());
        switch (handle) {
        case HandleTopLeft:
        case HandleBottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case HandleTopRight:
        case HandleBottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case HandleTop:
        case HandleBottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case HandleLeft:
        case HandleRight:
            setCursor(Qt::SizeHorCursor);
            break;
        default:
            setCursor(Qt::OpenHandCursor);
            break;
        }
    } else {
        setCursor(Qt::OpenHandCursor);
    }
    QGraphicsItem::hoverMoveEvent(event);
}

void SubComponent::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::hoverLeaveEvent(event);
}

QVariant SubComponent::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene()) {
        // Constrain position to stay within parent boundary
        QPointF newPos = value.toPointF();
        QPointF constrainedPos = constrainToBoundary(newPos);
        return constrainedPos;
    }
    return QGraphicsItem::itemChange(change, value);
}
