#include "subcomponent.h"
#include "component.h"
#include <QPainter>
#include <QCursor>
#include <QLinearGradient>

// Out-of-class definitions for static constexpr members (required for ODR-use in C++14)
constexpr qreal SubComponent::HANDLE_SIZE;
constexpr qreal SubComponent::MIN_WIDTH;
constexpr qreal SubComponent::MIN_HEIGHT;

SubComponent::SubComponent(SubComponentType type, const QString& text, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_type(type)
    , m_text(text)
    , m_width(100)
    , m_height(30)
    , m_activeHandle(HandleNone)
    , m_resizing(false)
{
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setFlag(ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);

    // Set default sizes based on type
    switch (type) {
    case SubComponentType::Label:
        m_width = 80;
        m_height = 24;
        break;
    case SubComponentType::LineEdit:
        m_width = 120;
        m_height = 28;
        break;
    case SubComponentType::Button:
        m_width = 90;
        m_height = 30;
        break;
    }
}

QRectF SubComponent::boundingRect() const
{
    // Include handle areas around the edges
    return QRectF(-HANDLE_SIZE / 2.0, -HANDLE_SIZE / 2.0,
                  m_width + HANDLE_SIZE, m_height + HANDLE_SIZE);
}

void SubComponent::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    switch (m_type) {
    case SubComponentType::Label:
        paintLabel(painter);
        break;
    case SubComponentType::LineEdit:
        paintLineEdit(painter);
        break;
    case SubComponentType::Button:
        paintButton(painter);
        break;
    }

    if (isSelected()) {
        paintResizeHandles(painter);
    }
}

// ---------------------------------------------------------------------------
// Visual rendering per subcomponent type
// ---------------------------------------------------------------------------

void SubComponent::paintLabel(QPainter* painter)
{
    // Subtle background
    painter->setPen(QPen(QColor(100, 105, 115), 1));
    painter->setBrush(QColor(52, 56, 63));
    painter->drawRoundedRect(0, 0, m_width, m_height, 3, 3);

    // Text
    painter->setPen(QColor(220, 222, 228));
    painter->setFont(QFont("Segoe UI", 8));
    painter->drawText(QRectF(6, 0, m_width - 12, m_height),
                      Qt::AlignVCenter | Qt::AlignLeft, m_text);
}

void SubComponent::paintLineEdit(QPainter* painter)
{
    // Input-field look: dark background, lighter inner area
    painter->setPen(QPen(QColor(90, 95, 105), 1));
    painter->setBrush(QColor(40, 42, 50));
    painter->drawRoundedRect(0, 0, m_width, m_height, 3, 3);

    // Placeholder text
    painter->setPen(QColor(140, 145, 155));
    painter->setFont(QFont("Segoe UI", 8));
    painter->drawText(QRectF(8, 0, m_width - 16, m_height),
                      Qt::AlignVCenter | Qt::AlignLeft, m_text);

    // Bottom accent line (focus hint)
    painter->setPen(QPen(QColor(33, 150, 243), 2));
    painter->drawLine(QPointF(2, m_height - 1), QPointF(m_width - 2, m_height - 1));
}

void SubComponent::paintButton(QPainter* painter)
{
    // Gradient button look
    QLinearGradient gradient(0, 0, 0, m_height);
    gradient.setColorAt(0, QColor(30, 110, 220));
    gradient.setColorAt(1, QColor(20, 85, 180));

    painter->setPen(QPen(QColor(15, 70, 160), 1));
    painter->setBrush(gradient);
    painter->drawRoundedRect(0, 0, m_width, m_height, 4, 4);

    // Centered white text
    painter->setPen(Qt::white);
    painter->setFont(QFont("Segoe UI", 8, QFont::Bold));
    painter->drawText(QRectF(0, 0, m_width, m_height),
                      Qt::AlignCenter, m_text);
}

void SubComponent::paintResizeHandles(QPainter* painter)
{
    // Selection dashed border
    painter->setPen(QPen(QColor(33, 150, 243), 1, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(0, 0, m_width, m_height);

    // Corner handles
    painter->setPen(QPen(Qt::white, 1));
    painter->setBrush(QColor(33, 150, 243));

    qreal hs = HANDLE_SIZE;
    QRectF handles[] = {
        QRectF(-hs / 2, -hs / 2, hs, hs),                             // TopLeft
        QRectF(m_width - hs / 2, -hs / 2, hs, hs),                    // TopRight
        QRectF(-hs / 2, m_height - hs / 2, hs, hs),                   // BottomLeft
        QRectF(m_width - hs / 2, m_height - hs / 2, hs, hs),          // BottomRight
    };

    for (const auto& rect : handles) {
        painter->drawRect(rect);
    }
}

// ---------------------------------------------------------------------------
// Resize handle hit-testing
// ---------------------------------------------------------------------------

SubComponent::ResizeHandle SubComponent::handleAt(const QPointF& pos) const
{
    qreal ht = HANDLE_SIZE * 2.0; // Hit-test tolerance (larger than visual handle)

    QRectF tl(-ht / 2, -ht / 2, ht, ht);
    QRectF tr(m_width - ht / 2, -ht / 2, ht, ht);
    QRectF bl(-ht / 2, m_height - ht / 2, ht, ht);
    QRectF br(m_width - ht / 2, m_height - ht / 2, ht, ht);

    if (tl.contains(pos)) return HandleTopLeft;
    if (tr.contains(pos)) return HandleTopRight;
    if (bl.contains(pos)) return HandleBottomLeft;
    if (br.contains(pos)) return HandleBottomRight;

    return HandleNone;
}

// ---------------------------------------------------------------------------
// Mouse events for resize and move
// ---------------------------------------------------------------------------

void SubComponent::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        ResizeHandle handle = handleAt(event->pos());
        if (handle != HandleNone) {
            m_activeHandle = handle;
            m_resizing = true;
            m_lastMouseScenePos = event->scenePos();
            event->accept();
            return;
        }
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

        qreal newX = pos().x();
        qreal newY = pos().y();
        qreal newW = m_width;
        qreal newH = m_height;

        switch (m_activeHandle) {
        case HandleTopLeft:
            newX += delta.x();
            newY += delta.y();
            newW -= delta.x();
            newH -= delta.y();
            break;
        case HandleTopRight:
            newY += delta.y();
            newW += delta.x();
            newH -= delta.y();
            break;
        case HandleBottomLeft:
            newX += delta.x();
            newW -= delta.x();
            newH += delta.y();
            break;
        case HandleBottomRight:
            newW += delta.x();
            newH += delta.y();
            break;
        default:
            break;
        }

        // Enforce minimum size
        if (newW < MIN_WIDTH) {
            if (m_activeHandle == HandleTopLeft || m_activeHandle == HandleBottomLeft)
                newX = pos().x() + m_width - MIN_WIDTH;
            newW = MIN_WIDTH;
        }
        if (newH < MIN_HEIGHT) {
            if (m_activeHandle == HandleTopLeft || m_activeHandle == HandleTopRight)
                newY = pos().y() + m_height - MIN_HEIGHT;
            newH = MIN_HEIGHT;
        }

        // Constrain to parent container if available
        Component* parentComp = dynamic_cast<Component*>(parentItem());
        if (parentComp) {
            QRectF container = parentComp->containerRect();
            // Ensure the new rect stays within the container
            if (newX < container.left()) {
                qreal diff = container.left() - newX;
                newX = container.left();
                newW -= diff;
                if (newW < MIN_WIDTH) newW = MIN_WIDTH;
            }
            if (newY < container.top() + 20) { // +20 for header
                qreal diff = (container.top() + 20) - newY;
                newY = container.top() + 20;
                newH -= diff;
                if (newH < MIN_HEIGHT) newH = MIN_HEIGHT;
            }
            if (newX + newW > container.right()) {
                newW = container.right() - newX;
                if (newW < MIN_WIDTH) newW = MIN_WIDTH;
            }
            if (newY + newH > container.bottom()) {
                newH = container.bottom() - newY;
                if (newH < MIN_HEIGHT) newH = MIN_HEIGHT;
            }
        }

        setPos(newX, newY);
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
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::mouseReleaseEvent(event);
}

// ---------------------------------------------------------------------------
// Hover events for resize cursor feedback
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Constrain position to parent container
// ---------------------------------------------------------------------------

QVariant SubComponent::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && parentItem()) {
        QPointF newPos = value.toPointF();

        Component* parentComp = dynamic_cast<Component*>(parentItem());
        if (parentComp) {
            QRectF container = parentComp->containerRect();
            qreal headerOffset = 20.0; // Skip container header

            qreal minX = container.left();
            qreal minY = container.top() + headerOffset;
            qreal maxX = container.right() - m_width;
            qreal maxY = container.bottom() - m_height;

            newPos.setX(qBound(minX, newPos.x(), qMax(minX, maxX)));
            newPos.setY(qBound(minY, newPos.y(), qMax(minY, maxY)));
        }

        return newPos;
    }
    return QGraphicsItem::itemChange(change, value);
}

// ---------------------------------------------------------------------------
// Mutators
// ---------------------------------------------------------------------------

void SubComponent::setText(const QString& text)
{
    m_text = text;
    update();
}

void SubComponent::setSize(qreal w, qreal h)
{
    prepareGeometryChange();
    m_width = qMax(w, MIN_WIDTH);
    m_height = qMax(h, MIN_HEIGHT);
    update();
}

// ---------------------------------------------------------------------------
// Serialization helpers
// ---------------------------------------------------------------------------

QString SubComponent::typeToString(SubComponentType type)
{
    switch (type) {
    case SubComponentType::Label:    return "Label";
    case SubComponentType::LineEdit: return "LineEdit";
    case SubComponentType::Button:   return "Button";
    }
    return "Label";
}

SubComponentType SubComponent::stringToType(const QString& str)
{
    if (str == "LineEdit") return SubComponentType::LineEdit;
    if (str == "Button")   return SubComponentType::Button;
    return SubComponentType::Label;
}
