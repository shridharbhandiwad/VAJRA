#include "component.h"
#include "componentregistry.h"
#include <QPainter>
#include <QCursor>
#include <QFileInfo>
#include <QDebug>
#include <QtMath>

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
}

QRectF Component::boundingRect() const
{
    qreal halfSize = m_size / 2.0;
    
    // Generous bounds that cover all possible drawing operations
    qreal top = -halfSize * 1.3;
    qreal bottom = halfSize * 2.5;
    qreal left = -halfSize * 1.3;
    qreal right = halfSize * 1.3;
    
    qreal width = right - left;
    qreal height = bottom - top;
    
    return QRectF(left, top, width, height);
}

void Component::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    if (m_hasImage && !m_image.isNull()) {
        paintWithImage(painter);
    } else {
        paintGeometric(painter);
    }
    
    // Draw selection border if selected
    if (isSelected()) {
        painter->setPen(QPen(QColor("#00BCD4"), 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
}

void Component::paintWithImage(QPainter* painter)
{
    qreal imageSize = m_size * 1.8;
    qreal imageHalfSize = imageSize / 2.0;
    
    // Draw card background with subtle shadow effect
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(30, 30, 35, 180));
    painter->drawRoundedRect(-imageHalfSize + 2, -imageHalfSize + 2, imageSize, imageSize, 8, 8);
    
    // Draw card
    painter->setPen(QPen(QColor(60, 65, 75), 1));
    painter->setBrush(QColor(36, 39, 46));
    painter->drawRoundedRect(-imageHalfSize, -imageHalfSize, imageSize, imageSize, 8, 8);
    
    // Draw the subsystem image with rounded corners
    QRectF imageRect(-imageHalfSize + 3, -imageHalfSize + 3, imageSize - 6, imageSize - 6);
    
    // Create rounded clip for image
    QPainterPath clipPath;
    clipPath.addRoundedRect(imageRect, 6, 6);
    painter->setClipPath(clipPath);
    painter->drawPixmap(imageRect.toRect(), m_image);
    painter->setClipping(false);
    
    // Draw health indicator bar at bottom of image
    QColor healthColor = m_color;
    qreal barWidth = imageSize - 6;
    qreal barHeight = 4;
    qreal barY = imageHalfSize - 7;
    
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 120));
    painter->drawRoundedRect(-imageHalfSize + 3, barY, barWidth, barHeight, 2, 2);
    painter->setBrush(healthColor);
    qreal healthFraction = qBound(0.0, m_size / 100.0, 1.0);
    painter->drawRoundedRect(-imageHalfSize + 3, barY, barWidth * healthFraction, barHeight, 2, 2);
    
    // Draw label below the image
    QString label = getLabel();
    painter->setFont(QFont("Segoe UI", 8, QFont::Bold));
    painter->setPen(QColor(200, 200, 210));
    painter->drawText(QRectF(-imageHalfSize, imageHalfSize + 3, imageSize, 14), Qt::AlignCenter, label);
}

void Component::paintGeometric(QPainter* painter)
{
    qreal halfSize = m_size / 2.0;
    
    // Get shape from registry
    QString shape = "rect";
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.hasComponent(m_typeId)) {
        shape = registry.getComponent(m_typeId).shape;
    }
    
    // Draw shadow
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 60));
    if (shape == "ellipse") {
        painter->drawEllipse(QRectF(-halfSize + 3, -halfSize + 3, m_size, m_size));
    } else {
        painter->drawRoundedRect(-halfSize + 3, -halfSize + 3, m_size, m_size, 6, 6);
    }
    
    // Draw main shape
    painter->setPen(QPen(m_color.darker(120), 2));
    
    // Create gradient fill
    QLinearGradient gradient(0, -halfSize, 0, halfSize);
    gradient.setColorAt(0, m_color.lighter(130));
    gradient.setColorAt(1, m_color);
    painter->setBrush(gradient);
    
    drawShape(painter, shape, halfSize);
    
    // Draw label below
    QString label = getLabel();
    painter->setFont(QFont("Segoe UI", 8, QFont::Bold));
    painter->setPen(QColor(200, 200, 210));
    painter->drawText(QRectF(-halfSize, halfSize + 4, m_size, 14), Qt::AlignCenter, label);
    
    // Draw health percentage inside
    painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
    painter->setPen(Qt::white);
    QString healthText = QString("%1%").arg(qRound(m_size));
    painter->drawText(QRectF(-halfSize, -halfSize, m_size, m_size), Qt::AlignCenter, healthText);
}

void Component::drawShape(QPainter* painter, const QString& shape, qreal halfSize)
{
    if (shape == "ellipse") {
        painter->drawEllipse(QRectF(-halfSize, -halfSize, m_size, m_size));
    } else if (shape == "hexagon") {
        // Draw hexagon
        QPolygonF hex;
        for (int i = 0; i < 6; ++i) {
            qreal angle = M_PI / 3.0 * i - M_PI / 6.0;
            hex << QPointF(halfSize * qCos(angle), halfSize * qSin(angle));
        }
        painter->drawPolygon(hex);
    } else if (shape == "diamond") {
        QPolygonF diamond;
        diamond << QPointF(0, -halfSize);
        diamond << QPointF(halfSize, 0);
        diamond << QPointF(0, halfSize);
        diamond << QPointF(-halfSize, 0);
        painter->drawPolygon(diamond);
    } else {
        // Default: rounded rectangle
        painter->drawRoundedRect(-halfSize, -halfSize, m_size, m_size, 6, 6);
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
        // Component position changed
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
