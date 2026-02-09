#include "component.h"
#include "componentregistry.h"
#include "thememanager.h"
#include <QPainter>
#include <QCursor>
#include <QFileInfo>
#include <QDebug>
#include <QtMath>
#include <QRandomGenerator>

// Out-of-class definitions for static constexpr members (required for ODR-use in C++14)
constexpr qreal Component::HEADER_HEIGHT;
constexpr qreal Component::PADDING;
constexpr qreal Component::SUB_SPACING;
constexpr qreal Component::MIN_WIDTH;
constexpr qreal Component::FOOTER_HEIGHT;
constexpr qreal Component::DESIGN_CONTAINER_HEADER;
constexpr qreal Component::DESIGN_CONTAINER_MIN_HEIGHT;
constexpr qreal Component::DESIGN_CONTAINER_FULL_HEIGHT;
constexpr qreal Component::RESIZE_HANDLE_SIZE;

Component::Component(const QString& typeId, const QString& id, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_typeId(typeId)
    , m_id(id)
    , m_color(Qt::blue)
    , m_size(50)
    , m_hasImage(false)
    , m_userWidth(0)
    , m_userHeight(0)
    , m_activeHandle(HandleNone)
    , m_resizing(false)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
    
    // Set initial color from registry definition
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.hasComponent(m_typeId)) {
        ComponentDefinition def = registry.getComponent(m_typeId);
        m_color = def.iconColor;
    }
    
    // Load subsystem image
    loadSubsystemImage();
    
    // Create default sub-components from registry definition
    createDefaultSubComponents();
}

void Component::createDefaultSubComponents()
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.hasComponent(m_typeId)) {
        ComponentDefinition def = registry.getComponent(m_typeId);
        for (int i = 0; i < def.subsystems.size(); ++i) {
            addSubComponent(def.subsystems[i]);
        }
    }
}

void Component::addSubComponent(const QString& name)
{
    int index = m_subComponents.size();
    SubComponent* sub = new SubComponent(name, index, this);
    m_subComponents.append(sub);
    
    prepareGeometryChange();
    layoutSubComponents();
}

void Component::removeSubComponent(int index)
{
    if (index >= 0 && index < m_subComponents.size()) {
        SubComponent* sub = m_subComponents.takeAt(index);
        delete sub;
        
        // Re-index remaining sub-components
        for (int i = 0; i < m_subComponents.size(); ++i) {
            m_subComponents[i]->setIndex(i);
        }
        
        prepareGeometryChange();
        layoutSubComponents();
    }
}

SubComponent* Component::getSubComponent(const QString& name) const
{
    for (SubComponent* sub : m_subComponents) {
        if (sub->getName() == name) {
            return sub;
        }
    }
    return nullptr;
}

// ======================================================================
// Design sub-component management (drag-drop widgets)
// ======================================================================

QRectF Component::designContainerRect() const
{
    // The design container sits below the subsystem sub-components area
    qreal y = HEADER_HEIGHT + PADDING;
    if (!m_subComponents.isEmpty()) {
        y += m_subComponents.size() * (SubComponent::itemHeight() + SUB_SPACING);
    } else {
        y += 30;
    }
    y += 4; // gap between subsystem area and design container
    
    qreal w = containerWidth();
    qreal defaultH = m_designSubComponents.isEmpty() ? DESIGN_CONTAINER_MIN_HEIGHT : DESIGN_CONTAINER_FULL_HEIGHT;
    
    // If the user resized the component, the design container gets all extra space
    qreal totalHeight = containerHeight();
    qreal availableH = totalHeight - y - FOOTER_HEIGHT;
    qreal h = qMax(defaultH, availableH);
    
    return QRectF(0, y, w, h);
}

bool Component::canAcceptDesignSubComponent(SubComponentType subType) const
{
    QStringList allowed = allowedWidgetTypes(m_typeId);
    QString typeName = DesignSubComponent::typeToString(subType);
    return allowed.contains(typeName);
}

void Component::addDesignSubComponent(DesignSubComponent* sub)
{
    if (!sub) return;
    prepareGeometryChange();
    sub->setParentItem(this);
    m_designSubComponents.append(sub);
    update();
}

void Component::removeDesignSubComponent(DesignSubComponent* sub)
{
    if (!sub) return;
    prepareGeometryChange();
    m_designSubComponents.removeOne(sub);
    update();
}

QStringList Component::allowedWidgetTypes(const QString& typeId)
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.hasComponent(typeId)) {
        ComponentDefinition def = registry.getComponent(typeId);
        if (!def.allowedWidgets.isEmpty()) {
            return def.allowedWidgets;
        }
    }
    // Default: all types allowed
    return QStringList() << "Label" << "LineEdit" << "Button";
}

QString Component::widgetValidationMessage(const QString& typeId, SubComponentType subType)
{
    QStringList allowed = allowedWidgetTypes(typeId);
    QString typeName = DesignSubComponent::typeToString(subType);
    
    if (allowed.contains(typeName)) {
        return QString(); // Valid
    }
    
    ComponentRegistry& registry = ComponentRegistry::instance();
    QString compName = typeId;
    if (registry.hasComponent(typeId)) {
        compName = registry.getComponent(typeId).displayName;
    }
    
    return QString("%1 cannot be placed inside %2.\nAllowed widget types: %3")
            .arg(typeName, compName, allowed.join(", "));
}

// ======================================================================
// Container dimensions
// ======================================================================

qreal Component::containerWidth() const
{
    qreal autoWidth = qMax(MIN_WIDTH, SubComponent::itemWidth() + PADDING * 2);
    // If user has manually resized, use the larger of auto and user width
    if (m_userWidth > 0) {
        return qMax(autoWidth, m_userWidth);
    }
    return autoWidth;
}

qreal Component::containerHeight() const
{
    qreal autoHeight = HEADER_HEIGHT + PADDING;
    
    if (!m_subComponents.isEmpty()) {
        autoHeight += m_subComponents.size() * (SubComponent::itemHeight() + SUB_SPACING);
    } else {
        autoHeight += 30; // Minimum content area
    }
    
    // Design container area (always present to serve as drop target)
    autoHeight += 4; // gap
    autoHeight += m_designSubComponents.isEmpty() ? DESIGN_CONTAINER_MIN_HEIGHT : DESIGN_CONTAINER_FULL_HEIGHT;
    
    autoHeight += FOOTER_HEIGHT;
    
    // If user has manually resized, use the larger of auto and user height
    if (m_userHeight > 0) {
        return qMax(autoHeight, m_userHeight);
    }
    return autoHeight;
}

void Component::layoutSubComponents()
{
    qreal yOffset = HEADER_HEIGHT + PADDING;
    qreal xOffset = PADDING;
    
    for (int i = 0; i < m_subComponents.size(); ++i) {
        m_subComponents[i]->setPos(xOffset, yOffset);
        yOffset += SubComponent::itemHeight() + SUB_SPACING;
    }
}

QPointF Component::anchorPoint() const
{
    return pos() + QPointF(containerWidth() / 2.0, containerHeight() / 2.0);
}

QRectF Component::boundingRect() const
{
    qreal w = containerWidth();
    qreal h = containerHeight();
    
    // Add extra space for resize handles and shadow
    qreal margin = RESIZE_HANDLE_SIZE;
    return QRectF(-margin, -margin, w + margin * 2, h + margin * 2);
}

void Component::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    paintContainer(painter);
    
    // Draw resize handles when selected
    if (isSelected()) {
        paintResizeHandles(painter);
    }
}

void Component::paintContainer(QPainter* painter)
{
    ThemeManager& tm = ThemeManager::instance();
    
    qreal w = containerWidth();
    qreal h = containerHeight();
    
    // Shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(tm.componentShadow());
    painter->drawRoundedRect(3, 3, w, h, 8, 8);
    
    // Main container background
    painter->setPen(QPen(tm.componentBorder(), 1.5));
    painter->setBrush(tm.componentBackground());
    painter->drawRoundedRect(0, 0, w, h, 8, 8);
    
    // Header background with component color
    QPainterPath headerPath;
    headerPath.addRoundedRect(0, 0, w, HEADER_HEIGHT, 8, 8);
    // Clip bottom corners of header
    headerPath.addRect(0, HEADER_HEIGHT - 8, w, 8);
    
    QLinearGradient headerGrad(0, 0, w, 0);
    if (tm.isDark()) {
        headerGrad.setColorAt(0, m_color.darker(180));
        headerGrad.setColorAt(1, m_color.darker(220));
    } else {
        headerGrad.setColorAt(0, m_color.lighter(160));
        headerGrad.setColorAt(1, m_color.lighter(140));
    }
    
    painter->setPen(Qt::NoPen);
    painter->setClipPath(headerPath);
    painter->setBrush(headerGrad);
    painter->drawRect(0, 0, w, HEADER_HEIGHT);
    painter->setClipping(false);
    
    // Header top border accent
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawRoundedRect(0, 0, w, 3, 2, 2);
    
    // Text colours for component header
    QColor headerTextPrimary = tm.isDark() ? QColor(230, 232, 237) : QColor(30, 35, 45);
    QColor headerTextSecondary = tm.isDark() ? m_color.lighter(150) : m_color.darker(140);
    
    // Component icon/thumbnail in header
    if (m_hasImage && !m_image.isNull()) {
        qreal imgSize = HEADER_HEIGHT - 10;
        QRectF imgRect(6, 5, imgSize, imgSize);
        QPainterPath clipPath;
        clipPath.addRoundedRect(imgRect, 4, 4);
        painter->setClipPath(clipPath);
        painter->drawPixmap(imgRect.toRect(), m_image);
        painter->setClipping(false);
        
        // Component name next to image
        painter->setPen(headerTextPrimary);
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        QRectF nameRect(6 + imgSize + 6, 2, w - imgSize - 18, HEADER_HEIGHT / 2);
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, getDisplayName());
        
        // Label/type ID below name
        painter->setPen(headerTextSecondary);
        painter->setFont(QFont("Segoe UI", 7, QFont::Normal));
        QRectF labelRect(6 + imgSize + 6, HEADER_HEIGHT / 2, w - imgSize - 18, HEADER_HEIGHT / 2 - 4);
        painter->drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft, getLabel());
    } else {
        // Geometric icon
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_color);
        painter->drawRoundedRect(8, 8, 24, 24, 4, 4);
        
        // Label inside icon
        painter->setPen(Qt::white);
        painter->setFont(QFont("Segoe UI", 7, QFont::Bold));
        painter->drawText(QRectF(8, 8, 24, 24), Qt::AlignCenter, getLabel());
        
        // Component name
        painter->setPen(headerTextPrimary);
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        QRectF nameRect(38, 2, w - 44, HEADER_HEIGHT / 2);
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, getDisplayName());
        
        // Health percentage
        painter->setPen(headerTextSecondary);
        painter->setFont(QFont("Segoe UI", 7, QFont::Bold));
        QRectF healthRect(38, HEADER_HEIGHT / 2, w - 44, HEADER_HEIGHT / 2 - 4);
        QString healthText = QString("Health: %1%").arg(qRound(m_size));
        painter->drawText(healthRect, Qt::AlignVCenter | Qt::AlignLeft, healthText);
    }
    
    // Health indicator bar below header
    qreal barY = HEADER_HEIGHT - 1;
    qreal barWidth = w;
    qreal barHeight = 3;
    
    painter->setPen(Qt::NoPen);
    painter->setBrush(tm.healthBarBackground());
    painter->drawRect(0, barY, barWidth, barHeight);
    
    qreal healthFraction = qBound(0.0, m_size / 100.0, 1.0);
    painter->setBrush(m_color);
    painter->drawRect(0, barY, barWidth * healthFraction, barHeight);
    
    // Sub-components section label (if there are sub-components)
    if (!m_subComponents.isEmpty()) {
        painter->setPen(tm.componentTextSecondary());
        painter->setFont(QFont("Segoe UI", 6, QFont::Bold));
        QRectF subLabelRect(PADDING, HEADER_HEIGHT + 1, w - PADDING * 2, PADDING - 1);
        painter->drawText(subLabelRect, Qt::AlignVCenter | Qt::AlignLeft, 
                          QString("SUB-SYSTEMS (%1)").arg(m_subComponents.size()));
    }
    
    // Design container area (for drag-drop widgets)
    QRectF dContainer = designContainerRect();
    
    if (!m_designSubComponents.isEmpty()) {
        // Container with visible border when widgets exist
        painter->setPen(QPen(tm.accentPrimary().darker(120), 1, Qt::DashLine));
        painter->setBrush(tm.isDark() ? QColor(25, 28, 35, 180) : QColor(240, 245, 252, 180));
        painter->drawRoundedRect(dContainer.adjusted(2, 0, -2, -2), 4, 4);
        
        // Header label
        painter->setPen(tm.accentPrimary());
        painter->setFont(QFont("Segoe UI", 6, QFont::Bold));
        painter->drawText(QRectF(dContainer.left() + PADDING, dContainer.top() + 2,
                                 dContainer.width() - PADDING * 2, DESIGN_CONTAINER_HEADER - 2),
                          Qt::AlignVCenter | Qt::AlignLeft,
                          QString("WIDGETS (%1)").arg(m_designSubComponents.size()));
    } else {
        // Subtle placeholder when empty (still a valid drop target)
        painter->setPen(QPen(tm.borderSubtle(), 1, Qt::DotLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(dContainer.adjusted(2, 0, -2, -2), 4, 4);
        
        painter->setPen(tm.mutedText());
        painter->setFont(QFont("Segoe UI", 6));
        painter->drawText(dContainer, Qt::AlignCenter, "Drop widgets here");
    }
}

QString Component::getDisplayName() const
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.hasComponent(m_typeId)) {
        return registry.getComponent(m_typeId).displayName;
    }
    return m_typeId;
}

QString Component::getLabel() const
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.hasComponent(m_typeId)) {
        return registry.getComponent(m_typeId).label;
    }
    // Generate a short label from typeId
    return m_typeId.left(4).toUpper();
}

void Component::setColor(const QColor& color)
{
    m_color = color;
    
    // Propagate color to sub-components
    for (SubComponent* sub : m_subComponents) {
        sub->setColor(color);
    }
    
    update();
}

void Component::setSize(qreal size)
{
    prepareGeometryChange();
    m_size = size;
    
    // Distribute health proportionally to sub-components
    for (SubComponent* sub : m_subComponents) {
        // Each sub-component gets a slightly varied health based on parent
        qreal variation = (QRandomGenerator::global()->bounded(20) - 10);  // +/- 10%
        qreal subHealth = qBound(0.0, size + variation, 100.0);
        sub->setHealth(subHealth);
    }
    
    update();
}

QString Component::toJson() const
{
    // Note: full JSON serialization with design sub-components is handled by Canvas::saveToJson()
    return QString("{\"id\":\"%1\",\"type\":\"%2\",\"x\":%3,\"y\":%4,\"color\":\"%5\",\"size\":%6,\"userWidth\":%7,\"userHeight\":%8}")
        .arg(m_id)
        .arg(m_typeId)
        .arg(pos().x())
        .arg(pos().y())
        .arg(m_color.name())
        .arg(m_size)
        .arg(m_userWidth)
        .arg(m_userHeight);
}

Component* Component::fromJson(const QString& id, const QString& typeId, qreal x, qreal y, 
                                const QColor& color, qreal size)
{
    Component* comp = new Component(typeId, id);
    comp->setPos(x, y);
    comp->setColor(color);
    comp->setSize(size);
    return comp;
}

void Component::setUserWidth(qreal w)
{
    prepareGeometryChange();
    m_userWidth = w;
    update();
}

void Component::setUserHeight(qreal h)
{
    prepareGeometryChange();
    m_userHeight = h;
    update();
}

// ======================================================================
// Resize handle hit-testing
// ======================================================================

Component::ResizeHandle Component::handleAt(const QPointF& pos) const
{
    if (!isSelected()) return HandleNone;
    
    qreal w = containerWidth();
    qreal h = containerHeight();
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

void Component::paintResizeHandles(QPainter* painter)
{
    qreal w = containerWidth();
    qreal h = containerHeight();
    qreal hs = RESIZE_HANDLE_SIZE;
    
    // Selection dashed border
    painter->setPen(QPen(QColor("#00BCD4"), 2, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(QRectF(0, 0, w, h), 8, 8);
    
    // Draw handle squares at corners and edges
    painter->setPen(QPen(Qt::white, 1));
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
// Mouse events for component resizing
// ======================================================================

void Component::mousePressEvent(QGraphicsSceneMouseEvent* event)
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
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsItem::mousePressEvent(event);
}

void Component::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_resizing && m_activeHandle != HandleNone) {
        QPointF delta = event->scenePos() - m_lastMouseScenePos;
        m_lastMouseScenePos = event->scenePos();
        
        prepareGeometryChange();
        
        qreal curW = containerWidth();
        qreal curH = containerHeight();
        qreal newX = pos().x();
        qreal newY = pos().y();
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
        qreal minAutoW = qMax(MIN_WIDTH, SubComponent::itemWidth() + PADDING * 2);
        qreal minAutoH = HEADER_HEIGHT + PADDING + 30 + 4 + DESIGN_CONTAINER_MIN_HEIGHT + FOOTER_HEIGHT;
        
        if (newW < minAutoW) {
            if (m_activeHandle == HandleTopLeft || m_activeHandle == HandleBottomLeft || m_activeHandle == HandleLeft)
                newX = pos().x() + curW - minAutoW;
            newW = minAutoW;
        }
        if (newH < minAutoH) {
            if (m_activeHandle == HandleTopLeft || m_activeHandle == HandleTopRight || m_activeHandle == HandleTop)
                newY = pos().y() + curH - minAutoH;
            newH = minAutoH;
        }
        
        setPos(newX, newY);
        m_userWidth = newW;
        m_userHeight = newH;
        
        layoutSubComponents();
        update();
        event->accept();
        return;
    }
    QGraphicsItem::mouseMoveEvent(event);
}

void Component::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
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

// ======================================================================
// Hover events for resize cursor feedback
// ======================================================================

void Component::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
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

void Component::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setCursor(Qt::OpenHandCursor);
    QGraphicsItem::hoverLeaveEvent(event);
}

QVariant Component::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene()) {
        // Component position changed - connections will be updated by the canvas
    }
    return QGraphicsItem::itemChange(change, value);
}

void Component::loadSubsystemImage()
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (!registry.hasComponent(m_typeId)) {
        return;
    }
    
    ComponentDefinition def = registry.getComponent(m_typeId);
    
    // Try JPG first
    QString imagePath = def.imagePath();
    QFileInfo checkFile(imagePath);
    if (checkFile.exists() && checkFile.isFile()) {
        m_image.load(imagePath);
        if (!m_image.isNull()) {
            m_hasImage = true;
            if (m_image.width() > 512 || m_image.height() > 512) {
                m_image = m_image.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            return;
        }
    }
    
    // Try PNG fallback
    imagePath = def.imagePathPng();
    QFileInfo checkFilePng(imagePath);
    if (checkFilePng.exists() && checkFilePng.isFile()) {
        m_image.load(imagePath);
        if (!m_image.isNull()) {
            m_hasImage = true;
            if (m_image.width() > 512 || m_image.height() > 512) {
                m_image = m_image.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    }
}
