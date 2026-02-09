#include "designsubcomponent.h"
#include "component.h"
#include "thememanager.h"
#include <QPainter>
#include <QCursor>
#include <QLinearGradient>

// Out-of-class definitions for static constexpr members (required for ODR-use in C++14)
constexpr qreal DesignSubComponent::HANDLE_SIZE;
constexpr qreal DesignSubComponent::MIN_WIDTH;
constexpr qreal DesignSubComponent::MIN_HEIGHT;

DesignSubComponent::DesignSubComponent(SubComponentType type, const QString& text, QGraphicsItem* parent)
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

QRectF DesignSubComponent::boundingRect() const
{
    // Include handle areas around the edges
    return QRectF(-HANDLE_SIZE / 2.0, -HANDLE_SIZE / 2.0,
                  m_width + HANDLE_SIZE, m_height + HANDLE_SIZE);
}

void DesignSubComponent::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
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
// Visual rendering per subcomponent type (theme-aware)
// ---------------------------------------------------------------------------

void DesignSubComponent::paintLabel(QPainter* painter)
{
    ThemeManager& tm = ThemeManager::instance();

    // Subtle background
    if (tm.isDark()) {
        painter->setPen(QPen(QColor(100, 105, 115), 1));
        painter->setBrush(QColor(52, 56, 63));
    } else {
        painter->setPen(QPen(QColor(180, 185, 195), 1));
        painter->setBrush(QColor(245, 247, 250));
    }
    painter->drawRoundedRect(0, 0, m_width, m_height, 3, 3);

    // Text
    painter->setPen(tm.isDark() ? QColor(220, 222, 228) : QColor(40, 45, 55));
    painter->setFont(QFont("Segoe UI", 8));
    painter->drawText(QRectF(6, 0, m_width - 12, m_height),
                      Qt::AlignVCenter | Qt::AlignLeft, m_text);
}

void DesignSubComponent::paintLineEdit(QPainter* painter)
{
    ThemeManager& tm = ThemeManager::instance();

    // Input-field look
    if (tm.isDark()) {
        painter->setPen(QPen(QColor(90, 95, 105), 1));
        painter->setBrush(QColor(40, 42, 50));
    } else {
        painter->setPen(QPen(QColor(180, 185, 195), 1));
        painter->setBrush(QColor(255, 255, 255));
    }
    painter->drawRoundedRect(0, 0, m_width, m_height, 3, 3);

    // Placeholder text
    painter->setPen(tm.isDark() ? QColor(140, 145, 155) : QColor(160, 165, 175));
    painter->setFont(QFont("Segoe UI", 8));
    painter->drawText(QRectF(8, 0, m_width - 16, m_height),
                      Qt::AlignVCenter | Qt::AlignLeft, m_text);

    // Bottom accent line (focus hint)
    painter->setPen(QPen(tm.accentPrimary(), 2));
    painter->drawLine(QPointF(2, m_height - 1), QPointF(m_width - 2, m_height - 1));
}

void DesignSubComponent::paintButton(QPainter* painter)
{
    ThemeManager& tm = ThemeManager::instance();

    // Gradient button look using accent color
    QColor baseColor = tm.accentPrimary();
    QLinearGradient gradient(0, 0, 0, m_height);
    gradient.setColorAt(0, baseColor.lighter(110));
    gradient.setColorAt(1, baseColor.darker(120));

    painter->setPen(QPen(baseColor.darker(140), 1));
    painter->setBrush(gradient);
    painter->drawRoundedRect(0, 0, m_width, m_height, 4, 4);

    // Centered white text
    painter->setPen(Qt::white);
    painter->setFont(QFont("Segoe UI", 8, QFont::Bold));
    painter->drawText(QRectF(0, 0, m_width, m_height),
                      Qt::AlignCenter, m_text);
}

void DesignSubComponent::paintResizeHandles(QPainter* painter)
{
    ThemeManager& tm = ThemeManager::instance();
    QColor accent = tm.accentPrimary();

    // Selection dashed border
    painter->setPen(QPen(accent, 1, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(0, 0, m_width, m_height);

    // Corner handles
    painter->setPen(QPen(Qt::white, 1));
    painter->setBrush(accent);

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

DesignSubComponent::ResizeHandle DesignSubComponent::handleAt(const QPointF& pos) const
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

void DesignSubComponent::mousePressEvent(QGraphicsSceneMouseEvent* event)
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

void DesignSubComponent::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
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

        // Constrain to parent design container if available
        Component* parentComp = dynamic_cast<Component*>(parentItem());
        if (parentComp) {
            QRectF container = parentComp->designContainerRect();
            qreal headerOffset = 18.0; // skip container header

            if (newX < container.left()) {
                qreal diff = container.left() - newX;
                newX = container.left();
                newW -= diff;
                if (newW < MIN_WIDTH) newW = MIN_WIDTH;
            }
            if (newY < container.top() + headerOffset) {
                qreal diff = (container.top() + headerOffset) - newY;
                newY = container.top() + headerOffset;
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

void DesignSubComponent::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
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

void DesignSubComponent::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
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

void DesignSubComponent::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::hoverLeaveEvent(event);
}

// ---------------------------------------------------------------------------
// Constrain position to parent design container
// ---------------------------------------------------------------------------

QVariant DesignSubComponent::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && parentItem()) {
        QPointF newPos = value.toPointF();

        Component* parentComp = dynamic_cast<Component*>(parentItem());
        if (parentComp) {
            QRectF container = parentComp->designContainerRect();
            qreal headerOffset = 18.0; // Skip container header

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

void DesignSubComponent::setText(const QString& text)
{
    m_text = text;
    update();
}

void DesignSubComponent::setSize(qreal w, qreal h)
{
    prepareGeometryChange();
    m_width = qMax(w, MIN_WIDTH);
    m_height = qMax(h, MIN_HEIGHT);
    update();
}

// ---------------------------------------------------------------------------
// Serialization helpers
// ---------------------------------------------------------------------------

QString DesignSubComponent::typeToString(SubComponentType type)
{
    switch (type) {
    case SubComponentType::Label:    return "Label";
    case SubComponentType::LineEdit: return "LineEdit";
    case SubComponentType::Button:   return "Button";
    }
    return "Label";
}

SubComponentType DesignSubComponent::stringToType(const QString& str)
{
    if (str == "LineEdit") return SubComponentType::LineEdit;
    if (str == "Button")   return SubComponentType::Button;
    return SubComponentType::Label;
}
