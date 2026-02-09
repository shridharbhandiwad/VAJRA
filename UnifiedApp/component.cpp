#include "component.h"
#include "componentregistry.h"
#include <QPainter>
#include <QCursor>
#include <QFileInfo>
#include <QDebug>
#include <QtMath>
#include <QRandomGenerator>

Component::Component(const QString& typeId, const QString& id, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_typeId(typeId)
    , m_id(id)
    , m_color(Qt::blue)
    , m_size(50)
    , m_hasImage(false)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
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

qreal Component::containerWidth() const
{
    qreal subWidth = SubComponent::itemWidth() + PADDING * 2;
    return qMax(MIN_WIDTH, subWidth);
}

qreal Component::containerHeight() const
{
    qreal height = HEADER_HEIGHT + PADDING;
    
    if (!m_subComponents.isEmpty()) {
        height += m_subComponents.size() * (SubComponent::itemHeight() + SUB_SPACING);
    } else {
        height += 30; // Minimum content area
    }
    
    height += FOOTER_HEIGHT;
    return height;
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
    
    // Add extra space for label below
    return QRectF(-2, -2, w + 4, h + 4);
}

void Component::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    paintContainer(painter);
    
    // Draw selection border if selected
    if (isSelected()) {
        painter->setPen(QPen(QColor("#00BCD4"), 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(boundingRect().adjusted(1, 1, -1, -1), 8, 8);
    }
}

void Component::paintContainer(QPainter* painter)
{
    qreal w = containerWidth();
    qreal h = containerHeight();
    
    // Shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 50));
    painter->drawRoundedRect(3, 3, w, h, 8, 8);
    
    // Main container background
    painter->setPen(QPen(QColor(55, 60, 70), 1.5));
    painter->setBrush(QColor(28, 30, 38));
    painter->drawRoundedRect(0, 0, w, h, 8, 8);
    
    // Header background with component color
    QPainterPath headerPath;
    headerPath.addRoundedRect(0, 0, w, HEADER_HEIGHT, 8, 8);
    // Clip bottom corners of header
    headerPath.addRect(0, HEADER_HEIGHT - 8, w, 8);
    
    QLinearGradient headerGrad(0, 0, w, 0);
    headerGrad.setColorAt(0, m_color.darker(180));
    headerGrad.setColorAt(1, m_color.darker(220));
    
    painter->setPen(Qt::NoPen);
    painter->setClipPath(headerPath);
    painter->setBrush(headerGrad);
    painter->drawRect(0, 0, w, HEADER_HEIGHT);
    painter->setClipping(false);
    
    // Header top border accent
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawRoundedRect(0, 0, w, 3, 2, 2);
    
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
        painter->setPen(QColor(230, 232, 237));
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        QRectF nameRect(6 + imgSize + 6, 2, w - imgSize - 18, HEADER_HEIGHT / 2);
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, getDisplayName());
        
        // Label/type ID below name
        painter->setPen(QColor(m_color.lighter(150)));
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
        painter->setPen(QColor(230, 232, 237));
        painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
        QRectF nameRect(38, 2, w - 44, HEADER_HEIGHT / 2);
        painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, getDisplayName());
        
        // Health percentage
        painter->setPen(m_color.lighter(130));
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
    painter->setBrush(QColor(20, 22, 28));
    painter->drawRect(0, barY, barWidth, barHeight);
    
    qreal healthFraction = qBound(0.0, m_size / 100.0, 1.0);
    painter->setBrush(m_color);
    painter->drawRect(0, barY, barWidth * healthFraction, barHeight);
    
    // Sub-components section label (if there are sub-components)
    if (!m_subComponents.isEmpty()) {
        painter->setPen(QColor(120, 125, 135));
        painter->setFont(QFont("Segoe UI", 6, QFont::Bold));
        QRectF subLabelRect(PADDING, HEADER_HEIGHT + 1, w - PADDING * 2, PADDING - 1);
        painter->drawText(subLabelRect, Qt::AlignVCenter | Qt::AlignLeft, 
                          QString("SUB-SYSTEMS (%1)").arg(m_subComponents.size()));
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
    return QString("{\"id\":\"%1\",\"type\":\"%2\",\"x\":%3,\"y\":%4,\"color\":\"%5\",\"size\":%6}")
        .arg(m_id)
        .arg(m_typeId)
        .arg(pos().x())
        .arg(pos().y())
        .arg(m_color.name())
        .arg(m_size);
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
