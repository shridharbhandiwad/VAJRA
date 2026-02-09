#include "component.h"
#include <QPainter>
#include <QCursor>
#include <QFileInfo>

Component::Component(ComponentType type, const QString& id, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_type(type)
    , m_id(id)
    , m_color(Qt::blue)
    , m_size(50)
    , m_hasImage(false)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setCursor(Qt::OpenHandCursor);
    
    // Load subsystem image
    loadSubsystemImage();
}

// ======================================================================
// Geometry
// ======================================================================

QRectF Component::boundingRect() const
{
    qreal halfSize = m_size / 2.0;
    
    // Base bounds (same as original – covers all geometric shapes + text)
    qreal top    = -halfSize * 1.3;
    qreal bottom =  halfSize * 2.5;
    qreal left   = -halfSize * 1.2;
    qreal right  =  halfSize * 1.2;
    
    // Expand to include the container area when subcomponents are present
    if (!m_subComponents.isEmpty()) {
        QRectF container = containerRect();
        left   = qMin(left,   container.left()   - 2);
        right  = qMax(right,  container.right()  + 2);
        bottom = qMax(bottom, container.bottom() + 2);
    }
    
    return QRectF(left, top, right - left, bottom - top);
}

QRectF Component::containerRect() const
{
    // The container is a fixed-size area below the main visual
    // where subcomponents are placed.
    qreal containerWidth  = 200.0;
    qreal containerHeight = 180.0;
    qreal containerTop    = m_size * 1.3 + 8;  // gap below main visual
    
    return QRectF(-containerWidth / 2.0, containerTop,
                   containerWidth, containerHeight);
}

// ======================================================================
// Painting
// ======================================================================

void Component::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    // --- Container area (drawn behind the main visual) ---
    if (!m_subComponents.isEmpty()) {
        QRectF container = containerRect();
        
        // Background
        painter->setPen(QPen(QColor(58, 63, 75), 1, Qt::DashLine));
        painter->setBrush(QColor(30, 32, 38, 210));
        painter->drawRoundedRect(container, 6, 6);
        
        // Header text
        painter->setPen(QColor(160, 165, 175));
        painter->setFont(QFont("Segoe UI", 7, QFont::Bold));
        painter->drawText(QRectF(container.left() + 8, container.top() + 2,
                                 container.width() - 16, 18),
                          Qt::AlignLeft | Qt::AlignVCenter, "SUB-COMPONENTS");
        
        // Thin connecting line from visual to container
        painter->setPen(QPen(QColor(58, 63, 75), 1, Qt::DotLine));
        qreal halfSize = m_size / 2.0;
        painter->drawLine(QPointF(0, halfSize * 2.5),
                          QPointF(0, container.top()));
    }
    
    // --- Main component visual ---
    if (m_hasImage && !m_image.isNull()) {
        qreal imageSize     = m_size * 1.8;
        qreal imageHalfSize = imageSize / 2.0;
        
        painter->setPen(QPen(Qt::black, 1));
        painter->setBrush(QColor(240, 240, 240));
        painter->drawRoundedRect(-imageHalfSize, -imageHalfSize,
                                  imageSize, imageSize, 5, 5);
        
        QRectF imageRect(-imageHalfSize + 2, -imageHalfSize + 2,
                          imageSize - 4, imageSize - 4);
        painter->drawPixmap(imageRect.toRect(), m_image);
        
        painter->setFont(QFont("Arial", 8, QFont::Bold));
        painter->setPen(Qt::black);
        QString label;
        switch (m_type) {
            case ComponentType::Antenna:             label = "ANT";  break;
            case ComponentType::PowerSystem:          label = "PWR";  break;
            case ComponentType::LiquidCoolingUnit:    label = "COOL"; break;
            case ComponentType::CommunicationSystem:  label = "COMM"; break;
            case ComponentType::RadarComputer:        label = "CPU";  break;
        }
        painter->drawText(QRectF(-imageHalfSize, imageHalfSize + 2,
                                  imageSize, 14),
                          Qt::AlignCenter, label);
    } else {
        paintGeometric(painter);
    }
    
    // --- Selection border ---
    if (isSelected()) {
        painter->setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
}

void Component::paintGeometric(QPainter* painter)
{
    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(m_color);
    
    qreal halfSize = m_size / 2.0;
    
    switch (m_type) {
        case ComponentType::Antenna: {
            painter->drawEllipse(-halfSize, -halfSize * 0.6, m_size, m_size * 0.6);
            painter->drawLine(0, halfSize * 0.3, 0, halfSize);
            painter->drawLine(-halfSize * 0.4, halfSize, halfSize * 0.4, halfSize);
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12),
                              Qt::AlignCenter, "ANT");
            break;
        }
        case ComponentType::PowerSystem: {
            painter->drawRect(-halfSize, -halfSize * 0.7, m_size, m_size * 1.4);
            painter->drawRect(-halfSize * 0.3, -halfSize * 0.9, m_size * 0.6, m_size * 0.2);
            QPen yellowPen(Qt::yellow, 3);
            painter->setPen(yellowPen);
            painter->drawLine(-halfSize * 0.3, -halfSize * 0.3, 0, 0);
            painter->drawLine(0, 0, halfSize * 0.3, halfSize * 0.3);
            painter->setPen(QPen(Qt::black, 2));
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12),
                              Qt::AlignCenter, "PWR");
            break;
        }
        case ComponentType::LiquidCoolingUnit: {
            painter->drawEllipse(-halfSize, -halfSize, m_size, m_size);
            painter->drawRect(-halfSize * 0.6, -halfSize * 1.2, m_size * 0.25, m_size * 0.25);
            painter->drawRect(halfSize * 0.35, -halfSize * 1.2, m_size * 0.25, m_size * 0.25);
            painter->drawLine(-halfSize * 0.5, -halfSize * 0.95, -halfSize * 0.5, -halfSize * 0.7);
            painter->drawLine(halfSize * 0.5, -halfSize * 0.95, halfSize * 0.5, -halfSize * 0.7);
            QPen bluePen(Qt::cyan, 2);
            painter->setPen(bluePen);
            painter->drawLine(-halfSize * 0.3, 0, halfSize * 0.3, 0);
            painter->drawLine(0, -halfSize * 0.3, 0, halfSize * 0.3);
            painter->setPen(QPen(Qt::black, 2));
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12),
                              Qt::AlignCenter, "COOL");
            break;
        }
        case ComponentType::CommunicationSystem: {
            painter->drawRect(-halfSize * 0.6, -halfSize * 0.6, m_size * 1.2, m_size * 1.2);
            for (int i = 1; i <= 3; i++) {
                qreal radius = halfSize * 0.3 * i;
                painter->drawArc(-radius, -radius, radius * 2, radius * 2, 45 * 16, 90 * 16);
            }
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12),
                              Qt::AlignCenter, "COMM");
            break;
        }
        case ComponentType::RadarComputer: {
            painter->drawRect(-halfSize, -halfSize * 0.8, m_size, m_size * 1.6);
            painter->drawLine(-halfSize * 0.5, -halfSize * 0.4, halfSize * 0.5, -halfSize * 0.4);
            painter->drawLine(-halfSize * 0.5, 0, halfSize * 0.5, 0);
            painter->drawLine(-halfSize * 0.5, halfSize * 0.4, halfSize * 0.5, halfSize * 0.4);
            painter->drawEllipse(-halfSize * 0.3, -halfSize * 0.2, m_size * 0.2, m_size * 0.2);
            painter->drawEllipse(halfSize * 0.1, -halfSize * 0.2, m_size * 0.2, m_size * 0.2);
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12),
                              Qt::AlignCenter, "CPU");
            break;
        }
    }
}

// ======================================================================
// Sub-component management
// ======================================================================

bool Component::canAcceptSubComponent(SubComponentType subType) const
{
    QList<SubComponentType> allowed = allowedSubComponentTypes(m_type);
    return allowed.contains(subType);
}

void Component::addSubComponent(SubComponent* sub)
{
    if (!sub) return;
    prepareGeometryChange();
    sub->setParentItem(this);
    m_subComponents.append(sub);
    update();
}

void Component::removeSubComponent(SubComponent* sub)
{
    if (!sub) return;
    prepareGeometryChange();
    m_subComponents.removeOne(sub);
    update();
}

QList<SubComponentType> Component::allowedSubComponentTypes(ComponentType compType)
{
    QList<SubComponentType> allowed;
    
    switch (compType) {
    case ComponentType::Antenna:
        // Antenna: Label and Button only – no text input
        allowed << SubComponentType::Label << SubComponentType::Button;
        break;
    case ComponentType::PowerSystem:
        // Power System: all types (configurable parameters)
        allowed << SubComponentType::Label << SubComponentType::LineEdit << SubComponentType::Button;
        break;
    case ComponentType::LiquidCoolingUnit:
        // Liquid Cooling: Label and Button only
        allowed << SubComponentType::Label << SubComponentType::Button;
        break;
    case ComponentType::CommunicationSystem:
        // Communication System: all types (needs text fields)
        allowed << SubComponentType::Label << SubComponentType::LineEdit << SubComponentType::Button;
        break;
    case ComponentType::RadarComputer:
        // Radar Computer: all types (configurable parameters)
        allowed << SubComponentType::Label << SubComponentType::LineEdit << SubComponentType::Button;
        break;
    }
    
    return allowed;
}

QString Component::validationMessage(ComponentType compType, SubComponentType subType)
{
    if (allowedSubComponentTypes(compType).contains(subType)) {
        return QString(); // Valid – no error
    }
    
    QString compName = componentTypeName(compType);
    QString subName  = SubComponent::typeToString(subType);
    
    QStringList allowedNames;
    foreach (SubComponentType t, allowedSubComponentTypes(compType)) {
        allowedNames << SubComponent::typeToString(t);
    }
    
    return QString("%1 cannot be placed inside %2.\nAllowed sub-component types: %3")
            .arg(subName, compName, allowedNames.join(", "));
}

// ======================================================================
// Property setters
// ======================================================================

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

// ======================================================================
// Serialization
// ======================================================================

QString Component::toJson() const
{
    QString typeStr;
    switch (m_type) {
        case ComponentType::Antenna:             typeStr = "Antenna"; break;
        case ComponentType::PowerSystem:          typeStr = "PowerSystem"; break;
        case ComponentType::LiquidCoolingUnit:    typeStr = "LiquidCoolingUnit"; break;
        case ComponentType::CommunicationSystem:  typeStr = "CommunicationSystem"; break;
        case ComponentType::RadarComputer:        typeStr = "RadarComputer"; break;
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
        // Component position changed – children follow automatically
    }
    return QGraphicsItem::itemChange(change, value);
}

// ======================================================================
// Helpers
// ======================================================================

QString Component::componentTypeName(ComponentType type)
{
    switch (type) {
        case ComponentType::Antenna:             return "Antenna";
        case ComponentType::PowerSystem:          return "Power System";
        case ComponentType::LiquidCoolingUnit:    return "Liquid Cooling Unit";
        case ComponentType::CommunicationSystem:  return "Communication System";
        case ComponentType::RadarComputer:        return "Radar Computer";
        default: return "Unknown";
    }
}

void Component::loadSubsystemImage()
{
    QString dirName = getSubsystemDirName(m_type);
    QString imagePath = QString("assets/subsystems/%1/%2_main.jpg").arg(dirName).arg(dirName);
    
    QFileInfo checkFile(imagePath);
    if (checkFile.exists() && checkFile.isFile()) {
        m_image.load(imagePath);
        if (!m_image.isNull()) {
            m_hasImage = true;
            if (m_image.width() > 512 || m_image.height() > 512) {
                m_image = m_image.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    } else {
        imagePath = QString("assets/subsystems/%1/%2_main.png").arg(dirName).arg(dirName);
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
}

QString Component::getSubsystemDirName(ComponentType type)
{
    switch (type) {
        case ComponentType::Antenna:             return "antenna";
        case ComponentType::PowerSystem:          return "power_system";
        case ComponentType::LiquidCoolingUnit:    return "liquid_cooling_unit";
        case ComponentType::CommunicationSystem:  return "communication_system";
        case ComponentType::RadarComputer:        return "radar_computer";
        default: return "antenna";
    }
}
