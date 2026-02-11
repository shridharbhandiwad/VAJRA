#include "designsubcomponent.h"
#include "component.h"
#include "canvas.h"
#include "thememanager.h"
#include <QPainter>
#include <QCursor>
#include <QLinearGradient>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QGraphicsView>
#include <QGraphicsScene>

// Out-of-class definitions for static constexpr members (required for ODR-use in C++14)
constexpr qreal DesignSubComponent::HANDLE_SIZE;
constexpr qreal DesignSubComponent::MIN_WIDTH;
constexpr qreal DesignSubComponent::MIN_HEIGHT;
constexpr qreal DesignSubComponent::HEALTH_BAR_WIDTH;

DesignSubComponent::DesignSubComponent(SubComponentType type, const QString& text, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_type(type)
    , m_text(text)
    , m_width(100)
    , m_height(30)
    , m_activeHandle(HandleNone)
    , m_resizing(false)
    , m_healthColor(QColor(76, 175, 80))  // Default green (healthy)
    , m_healthValue(100.0)
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
        // Only show resize handles in Designer mode
        Canvas* canvas = nullptr;
        if (scene()) {
            canvas = qobject_cast<Canvas*>(scene()->parent());
        }
        if (canvas && canvas->getUserRole() == UserRole::Designer) {
            paintResizeHandles(painter);
        }
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

    // Health indicator bar on the left
    paintHealthIndicator(painter);

    // Text (offset to make room for health indicator bar)
    painter->setPen(tm.isDark() ? QColor(220, 222, 228) : QColor(40, 45, 55));
    painter->setFont(QFont("Inter", 10));
    painter->drawText(QRectF(HEALTH_BAR_WIDTH + 6, 0, m_width - HEALTH_BAR_WIDTH - 12, m_height),
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

    // Health indicator bar on the left
    paintHealthIndicator(painter);

    // Placeholder text
    painter->setPen(tm.isDark() ? QColor(140, 145, 155) : QColor(160, 165, 175));
    painter->setFont(QFont("Inter", 10));
    painter->drawText(QRectF(HEALTH_BAR_WIDTH + 8, 0, m_width - HEALTH_BAR_WIDTH - 16, m_height),
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

    // Health indicator bar on the left
    paintHealthIndicator(painter);

    // Centered white text
    painter->setPen(Qt::white);
    painter->setFont(QFont("Inter", 10, QFont::Bold));
    painter->drawText(QRectF(0, 0, m_width, m_height),
                      Qt::AlignCenter, m_text);
}

void DesignSubComponent::paintHealthIndicator(QPainter* painter)
{
    // Health indicator bar on the left edge of the subcomponent
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_healthColor);
    painter->drawRoundedRect(0, 0, HEALTH_BAR_WIDTH, m_height, 2, 2);
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
        // Only allow resizing in Designer mode
        Canvas* canvas = nullptr;
        if (scene()) {
            canvas = qobject_cast<Canvas*>(scene()->parent());
        }
        if (canvas && canvas->getUserRole() == UserRole::Designer) {
            ResizeHandle handle = handleAt(event->pos());
            if (handle != HandleNone) {
                m_activeHandle = handle;
                m_resizing = true;
                m_lastMouseScenePos = event->scenePos();
                event->accept();
                return;
            }
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
        // Only show resize cursors in Designer mode
        Canvas* canvas = nullptr;
        if (scene()) {
            canvas = qobject_cast<Canvas*>(scene()->parent());
        }
        if (canvas && canvas->getUserRole() == UserRole::Designer) {
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
// Double-click to edit text
// ---------------------------------------------------------------------------

void DesignSubComponent::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    showTextEditDialog();
}

void DesignSubComponent::showTextEditDialog()
{
    // Get the parent widget from the scene's view
    QWidget* parentWidget = nullptr;
    if (scene() && !scene()->views().isEmpty()) {
        parentWidget = scene()->views().first();
    }

    bool ok = false;
    QString newText = QInputDialog::getText(
        parentWidget,
        "Edit Sub-Component Text",
        "Enter new text:",
        QLineEdit::Normal,
        m_text,
        &ok);

    if (ok && !newText.isEmpty()) {
        setText(newText);
    }
}

// ---------------------------------------------------------------------------
// Right-click context menu
// ---------------------------------------------------------------------------

void DesignSubComponent::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;

    QAction* editAction = menu.addAction("Edit Text...");
    menu.addSeparator();

    QAction* setLabelAction = menu.addAction("Change to Label");
    QAction* setLineEditAction = menu.addAction("Change to LineEdit");
    QAction* setButtonAction = menu.addAction("Change to Button");
    menu.addSeparator();

    QAction* deleteAction = menu.addAction("Delete");

    // Disable the type that is already selected
    switch (m_type) {
    case SubComponentType::Label:    setLabelAction->setEnabled(false); break;
    case SubComponentType::LineEdit: setLineEditAction->setEnabled(false); break;
    case SubComponentType::Button:   setButtonAction->setEnabled(false); break;
    }

    QAction* selected = menu.exec(event->screenPos());

    if (selected == editAction) {
        showTextEditDialog();
    } else if (selected == setLabelAction) {
        m_type = SubComponentType::Label;
        update();
    } else if (selected == setLineEditAction) {
        m_type = SubComponentType::LineEdit;
        update();
    } else if (selected == setButtonAction) {
        m_type = SubComponentType::Button;
        update();
    } else if (selected == deleteAction) {
        Component* parentComp = dynamic_cast<Component*>(parentItem());
        if (parentComp) {
            parentComp->removeDesignSubComponent(this);
        }
        delete this;
        return;
    }
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

void DesignSubComponent::setHealthColor(const QColor& color)
{
    m_healthColor = color;
    update();
}

void DesignSubComponent::setHealthValue(qreal value)
{
    m_healthValue = qBound(0.0, value, 100.0);
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
