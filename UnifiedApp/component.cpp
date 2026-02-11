#include "component.h"
#include "componentregistry.h"
#include "thememanager.h"
#include "editcomponentdialog.h"
#include "canvas.h"
#include <QPainter>
#include <QCursor>
#include <QFileInfo>
#include <QDebug>
#include <QtMath>
#include <QRandomGenerator>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsScene>

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
    , m_minimized(false)
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
    
    // Position the new subcomponent in a 2xN grid layout for better visual appearance
    QRectF container = designContainerRect();
    qreal subWidth = SubComponent::defaultWidth();
    qreal subHeight = SubComponent::defaultHeight();
    qreal horizontalSpacing = 15;  // Space between columns
    qreal verticalSpacing = 10;     // Space between rows
    qreal leftMargin = 15;          // Left margin
    qreal topMargin = 25;           // Top margin
    
    int column = index % 2;  // 2 columns (0 or 1)
    int row = index / 2;      // Row number
    
    qreal xOffset = container.left() + leftMargin + column * (subWidth + horizontalSpacing);
    qreal yOffset = container.top() + topMargin + row * (subHeight + verticalSpacing);
    sub->setPos(xOffset, yOffset);
    
    prepareGeometryChange();
    update();
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
        update();
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
    // The design container sits below the header - this is where subcomponents can be freely placed
    qreal y = HEADER_HEIGHT + PADDING;
    qreal w = containerWidth();
    
    // Calculate height as everything below the header except the footer
    qreal totalHeight = containerHeight();
    qreal h = totalHeight - y - FOOTER_HEIGHT - 8;  // 8px margin at bottom
    
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
    // Calculate width needed for 2-column sub-component layout
    qreal subWidth = SubComponent::defaultWidth();
    qreal horizontalSpacing = 15;
    qreal leftMargin = 15;
    qreal rightMargin = 15;
    
    // Width for 2 columns of sub-components
    qreal widthFor2Columns = leftMargin + (2 * subWidth) + horizontalSpacing + rightMargin;
    qreal autoWidth = qMax(MIN_WIDTH, widthFor2Columns);
    
    // Check if any subcomponents have been resized or repositioned beyond the default layout
    for (SubComponent* sub : m_subComponents) {
        qreal subRight = sub->pos().x() + sub->getWidth();
        qreal requiredWidth = subRight + rightMargin;
        if (requiredWidth > autoWidth) {
            autoWidth = requiredWidth;
        }
    }
    
    // Also check design sub-components
    for (DesignSubComponent* dsub : m_designSubComponents) {
        qreal dsubRight = dsub->pos().x() + dsub->getWidth();
        qreal requiredWidth = dsubRight + rightMargin;
        if (requiredWidth > autoWidth) {
            autoWidth = requiredWidth;
        }
    }
    
    // If user has manually resized, use the larger of auto and user width
    if (m_userWidth > 0) {
        return qMax(autoWidth, m_userWidth);
    }
    return autoWidth;
}

qreal Component::containerHeight() const
{
    // If minimized, only show the header
    if (m_minimized) {
        return HEADER_HEIGHT;
    }
    
    qreal autoHeight = HEADER_HEIGHT + PADDING;
    
    // SubComponents section - calculate required height based on actual layout
    qreal subComponentsHeight = 60; // Minimum space for empty state
    
    if (!m_subComponents.isEmpty()) {
        // Calculate based on 2-column grid layout (matching addSubComponent logic)
        int numRows = (m_subComponents.size() + 1) / 2; // Ceiling division for 2 columns
        qreal subHeight = SubComponent::defaultHeight();
        qreal verticalSpacing = 10;
        qreal topMargin = 25;
        qreal bottomMargin = 15;
        
        subComponentsHeight = topMargin + (numRows * subHeight) + ((numRows - 1) * verticalSpacing) + bottomMargin;
        
        // Also check if any subcomponents have been resized or repositioned beyond the grid
        for (SubComponent* sub : m_subComponents) {
            qreal subBottom = sub->pos().y() + sub->getHeight();
            qreal requiredHeight = subBottom + bottomMargin;
            if (requiredHeight > subComponentsHeight) {
                subComponentsHeight = requiredHeight;
            }
        }
    }
    
    autoHeight += subComponentsHeight;
    
    // Design container area (always present to serve as drop target)
    autoHeight += 4; // gap
    qreal designContainerHeight = m_designSubComponents.isEmpty() ? DESIGN_CONTAINER_MIN_HEIGHT : DESIGN_CONTAINER_FULL_HEIGHT;
    
    // Check if any design sub-components extend beyond the default container height
    if (!m_designSubComponents.isEmpty()) {
        qreal bottomMargin = 15;
        for (DesignSubComponent* dsub : m_designSubComponents) {
            qreal dsubBottom = dsub->pos().y() + dsub->getHeight();
            // Position is relative to parent, so we need to account for the offset of the design container
            qreal containerTop = HEADER_HEIGHT + PADDING + subComponentsHeight + 4;
            qreal requiredHeight = dsubBottom - containerTop + bottomMargin;
            if (requiredHeight > designContainerHeight) {
                designContainerHeight = requiredHeight;
            }
        }
    }
    
    autoHeight += designContainerHeight;
    autoHeight += FOOTER_HEIGHT;
    
    // If user has manually resized, use the larger of auto and user height
    if (m_userHeight > 0) {
        return qMax(autoHeight, m_userHeight);
    }
    return autoHeight;
}

void Component::layoutSubComponents()
{
    // No longer auto-layout subcomponents - they are freely positioned by the user
    // This method is kept for API compatibility but does nothing
    // Subcomponents maintain their user-defined positions
}

void Component::notifySubComponentGeometryChange()
{
    prepareGeometryChange();
    update();
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
    
    // Draw minimize/maximize button
    paintMinimizeButton(painter);
    
    // Draw resize handles when selected and in Designer mode only
    if (isSelected() && !m_minimized) {
        Canvas* canvas = nullptr;
        if (scene()) {
            canvas = qobject_cast<Canvas*>(scene()->parent());
        }
        if (canvas && canvas->getUserRole() == UserRole::Designer) {
            paintResizeHandles(painter);
        }
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
    // Clip bottom corners of header only if not minimized
    if (!m_minimized) {
        headerPath.addRect(0, HEADER_HEIGHT - 8, w, 8);
    }
    
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
    
    // Text colours for component header using theme
    QColor headerTextPrimary = tm.componentTextPrimary();
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
        
        // Component name next to image (adjust width to leave room for button)
        painter->setPen(headerTextPrimary);
        painter->setFont(QFont("Inter", 11, QFont::Bold));
        QRectF nameRect(6 + imgSize + 6, 2, w - imgSize - 50, HEADER_HEIGHT / 2);
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, getDisplayName());
        
        // Label/type ID below name
        painter->setPen(headerTextSecondary);
        painter->setFont(QFont("Inter", 9, QFont::Normal));
        QRectF labelRect(6 + imgSize + 6, HEADER_HEIGHT / 2, w - imgSize - 50, HEADER_HEIGHT / 2 - 4);
        painter->drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft, getLabel());
    } else {
        // Geometric icon
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_color);
        painter->drawRoundedRect(8, 8, 24, 24, 4, 4);
        
        // Label inside icon
        painter->setPen(Qt::white);
        painter->setFont(QFont("Inter", 9, QFont::Bold));
        painter->drawText(QRectF(8, 8, 24, 24), Qt::AlignCenter, getLabel());
        
        // Component name (adjust width to leave room for button)
        painter->setPen(headerTextPrimary);
        painter->setFont(QFont("Inter", 11, QFont::Bold));
        QRectF nameRect(38, 2, w - 76, HEADER_HEIGHT / 2);
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, getDisplayName());
        
        // Health percentage
        painter->setPen(headerTextSecondary);
        painter->setFont(QFont("Inter", 9, QFont::Bold));
        QRectF healthRect(38, HEADER_HEIGHT / 2, w - 76, HEADER_HEIGHT / 2 - 4);
        QString healthText = QString("Health: %1%").arg(qRound(m_size));
        painter->drawText(healthRect, Qt::AlignVCenter | Qt::AlignLeft, healthText);
    }
    
    // Health indicator bar below header (only show when not minimized)
    if (!m_minimized) {
        qreal barY = HEADER_HEIGHT - 1;
        qreal barWidth = w;
        qreal barHeight = 3;
        
        painter->setPen(Qt::NoPen);
        painter->setBrush(tm.healthBarBackground());
        painter->drawRect(0, barY, barWidth, barHeight);
        
        qreal healthFraction = qBound(0.0, m_size / 100.0, 1.0);
        painter->setBrush(m_color);
        painter->drawRect(0, barY, barWidth * healthFraction, barHeight);
        
        // Workspace container area (for freely positioned subcomponents and widgets)
        QRectF dContainer = designContainerRect();
        
        // Draw a subtle container border
        painter->setPen(QPen(tm.borderSubtle(), 1, Qt::DotLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(dContainer.adjusted(2, 0, -2, -2), 4, 4);
        
        // Header label showing count of subcomponents and widgets
        int totalItems = m_subComponents.size() + m_designSubComponents.size();
        if (totalItems > 0) {
            painter->setPen(tm.componentTextSecondary());
            painter->setFont(QFont("Inter", 8, QFont::Bold));
            QRectF labelRect(PADDING, HEADER_HEIGHT + 1, w - PADDING * 2, PADDING - 1);
            QString label = QString("COMPONENTS (%1)").arg(totalItems);
            if (m_subComponents.size() > 0 && m_designSubComponents.size() > 0) {
                label = QString("SUB-SYSTEMS (%1) | WIDGETS (%2)")
                        .arg(m_subComponents.size())
                        .arg(m_designSubComponents.size());
            } else if (m_subComponents.size() > 0) {
                label = QString("SUB-SYSTEMS (%1)").arg(m_subComponents.size());
            } else if (m_designSubComponents.size() > 0) {
                label = QString("WIDGETS (%1)").arg(m_designSubComponents.size());
            }
            painter->drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft, label);
        } else {
            // Show placeholder when empty
            painter->setPen(tm.mutedText());
            painter->setFont(QFont("Inter", 8));
            painter->drawText(dContainer, Qt::AlignCenter, "Drag & drop components here");
        }
    }
}

void Component::paintMinimizeButton(QPainter* painter)
{
    ThemeManager& tm = ThemeManager::instance();
    qreal w = containerWidth();
    
    // Button position in top-right corner of header
    qreal buttonSize = 24;
    qreal buttonX = w - buttonSize - 8;
    qreal buttonY = (HEADER_HEIGHT - buttonSize) / 2;
    QRectF buttonRect(buttonX, buttonY, buttonSize, buttonSize);
    
    // Button background using theme
    QColor buttonBg = tm.componentHeaderOverlay();
    QColor buttonBorder = tm.borderLight();
    
    painter->setPen(QPen(buttonBorder, 1));
    painter->setBrush(buttonBg);
    painter->drawRoundedRect(buttonRect, 4, 4);
    
    // Button icon (minimize = horizontal line, maximize = square)
    painter->setPen(QPen(Qt::white, 2));
    painter->setBrush(Qt::NoBrush);
    
    if (m_minimized) {
        // Maximize icon (square)
        QRectF iconRect = buttonRect.adjusted(6, 6, -6, -6);
        painter->drawRect(iconRect);
    } else {
        // Minimize icon (horizontal line)
        qreal lineY = buttonRect.center().y();
        painter->drawLine(QPointF(buttonRect.left() + 6, lineY), 
                         QPointF(buttonRect.right() - 6, lineY));
    }
}

bool Component::isPointInMinimizeButton(const QPointF& pos) const
{
    qreal w = containerWidth();
    qreal buttonSize = 24;
    qreal buttonX = w - buttonSize - 8;
    qreal buttonY = (HEADER_HEIGHT - buttonSize) / 2;
    QRectF buttonRect(buttonX, buttonY, buttonSize, buttonSize);
    
    return buttonRect.contains(pos);
}

QString Component::getDisplayName() const
{
    // If per-component display name is set, use it
    if (!m_displayName.isEmpty()) {
        return m_displayName;
    }
    
    // Otherwise, fall back to registry default
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.hasComponent(m_typeId)) {
        return registry.getComponent(m_typeId).displayName;
    }
    return m_typeId;
}

QString Component::getLabel() const
{
    // If per-component label is set, use it
    if (!m_label.isEmpty()) {
        return m_label;
    }
    
    // Otherwise, fall back to registry default
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.hasComponent(m_typeId)) {
        return registry.getComponent(m_typeId).label;
    }
    // Generate a short label from typeId
    return m_typeId.left(4).toUpper();
}

void Component::setDisplayName(const QString& displayName)
{
    m_displayName = displayName;
    update();
}

void Component::setLabel(const QString& label)
{
    m_label = label;
    update();
}

void Component::setMinimized(bool minimized)
{
    if (m_minimized == minimized) {
        return;  // No change
    }
    
    prepareGeometryChange();
    m_minimized = minimized;
    updateSubComponentsVisibility();
    update();
}

void Component::updateSubComponentsVisibility()
{
    // Hide/show all subcomponents based on minimized state
    for (SubComponent* sub : m_subComponents) {
        sub->setVisible(!m_minimized);
    }
    
    for (DesignSubComponent* dsub : m_designSubComponents) {
        dsub->setVisible(!m_minimized);
    }
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
    return QString("{\"id\":\"%1\",\"type\":\"%2\",\"x\":%3,\"y\":%4,\"color\":\"%5\",\"size\":%6,\"userWidth\":%7,\"userHeight\":%8,\"minimized\":%9}")
        .arg(m_id)
        .arg(m_typeId)
        .arg(pos().x())
        .arg(pos().y())
        .arg(m_color.name())
        .arg(m_size)
        .arg(m_userWidth)
        .arg(m_userHeight)
        .arg(m_minimized ? "true" : "false");
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
    ThemeManager& tm = ThemeManager::instance();
    qreal w = containerWidth();
    qreal h = containerHeight();
    qreal hs = RESIZE_HANDLE_SIZE;
    
    // Selection dashed border using theme accent
    painter->setPen(QPen(tm.accentPrimary(), 2, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(QRectF(0, 0, w, h), 8, 8);
    
    // Draw handle squares at corners and edges
    painter->setPen(QPen(tm.invertedText(), 1));
    painter->setBrush(tm.accentPrimary());
    
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
    if (event->button() == Qt::LeftButton) {
        // Check if click is on minimize/maximize button
        if (isPointInMinimizeButton(event->pos())) {
            setMinimized(!m_minimized);
            event->accept();
            return;
        }
        
        // Only allow resizing in Designer mode when not minimized
        if (isSelected() && !m_minimized) {
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
        }
        
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
        qreal minAutoW = qMax(MIN_WIDTH, SubComponent::defaultWidth() + PADDING * 2);
        qreal minAutoH = HEADER_HEIGHT + PADDING + 60 + FOOTER_HEIGHT;
        
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

// ======================================================================
// Context menu for component editing
// ======================================================================

void Component::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;
    
    // Get user role from canvas to determine available options
    Canvas* canvas = nullptr;
    if (scene()) {
        canvas = qobject_cast<Canvas*>(scene()->parent());
    }
    
    QAction* editAction = nullptr;
    QAction* duplicateAction = nullptr;
    QAction* deleteAction = nullptr;
    
    // Only show "Edit Component", "Duplicate", and "Delete" options for Designer role
    if (canvas && canvas->getUserRole() == UserRole::Designer) {
        editAction = menu.addAction("✏️ Edit Component...");
        menu.addSeparator();
        
        duplicateAction = menu.addAction("📋 Duplicate");
        menu.addSeparator();
        
        deleteAction = menu.addAction("🗑️ Delete");
    }
    
    // Show context menu only if there are actions available
    if (!editAction && !duplicateAction && !deleteAction) {
        // No actions available (User mode)
        return;
    }
    
    QAction* selected = menu.exec(event->screenPos());
    
    if (editAction && selected == editAction) {
        // Open edit dialog
        EditComponentDialog dialog(this, canvas);
        if (dialog.exec() == QDialog::Accepted && dialog.hasChanges()) {
            // Changes have been applied to the component
            update();
            
            // Notify canvas that component has been edited
            if (canvas) {
                canvas->notifyComponentEdited(m_id, m_typeId);
            }
        }
    } else if (duplicateAction && selected == duplicateAction) {
        // TODO: Implement component duplication
        qDebug() << "[Component] Duplicate not yet implemented";
    } else if (deleteAction && selected == deleteAction) {
        // Delete this component
        if (scene()) {
            scene()->removeItem(this);
        }
        delete this;
    }
}
