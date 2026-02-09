#include "component.h"
#include <QPainter>
#include <QCursor>
#include <QDebug>
#include <QFileInfo>

Component::Component(ComponentType type, const QString& id, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_type(type)
    , m_id(id)
    , m_color(Qt::blue)
    , m_size(50)
    , m_hasImage(false)
{
    // Runtime components are not movable
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    
    // Load subsystem image
    loadSubsystemImage();
}

QRectF Component::boundingRect() const
{
    qreal halfSize = m_size / 2.0;
    
    // Calculate bounds that encompass all drawing operations for all component types
    // Different component types draw shapes that extend beyond the basic square
    
    // Maximum extents across all component types:
    // - Antenna: ellipse + lines + text
    // - PowerSystem: large rect (1.4x height) + top rect + text
    // - LiquidCoolingUnit: ellipse + top rects at -1.2x + text
    // - CommunicationSystem: rect (1.2x width/height) + arcs + text
    // - RadarComputer: large rect (1.6x height) + text
    
    // Text labels add ~14 pixels below (halfSize + 2 + 12)
    qreal top = -halfSize * 1.3;      // Covers top rects/decorations
    qreal bottom = halfSize * 2.5;     // Covers large rects + text
    qreal left = -halfSize * 1.2;      // Covers extended shapes
    qreal right = halfSize * 1.2;      // Covers extended shapes
    
    qreal width = right - left;
    qreal height = bottom - top;
    
    return QRectF(left, top, width, height);
}

void Component::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    qDebug() << "[Component]" << m_id << "- Painting with color" << m_color.name() << "and size" << m_size;
    
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    
    // If we have an image, draw it; otherwise fallback to geometric representation
    if (m_hasImage && !m_image.isNull()) {
        // Calculate image drawing area - make it slightly larger for better visibility
        qreal imageSize = m_size * 1.8;
        qreal imageHalfSize = imageSize / 2.0;
        
        // Draw a subtle background/border
        painter->setPen(QPen(Qt::black, 1));
        painter->setBrush(QColor(240, 240, 240));
        painter->drawRoundedRect(-imageHalfSize, -imageHalfSize, imageSize, imageSize, 5, 5);
        
        // Draw the subsystem image
        QRectF imageRect(-imageHalfSize + 2, -imageHalfSize + 2, imageSize - 4, imageSize - 4);
        painter->drawPixmap(imageRect.toRect(), m_image);
        
        // Draw label below the image
        painter->setFont(QFont("Arial", 8, QFont::Bold));
        painter->setPen(Qt::black);
        QString label;
        switch (m_type) {
            case ComponentType::Antenna: label = "ANT"; break;
            case ComponentType::PowerSystem: label = "PWR"; break;
            case ComponentType::LiquidCoolingUnit: label = "COOL"; break;
            case ComponentType::CommunicationSystem: label = "COMM"; break;
            case ComponentType::RadarComputer: label = "CPU"; break;
        }
        painter->drawText(QRectF(-imageHalfSize, imageHalfSize + 2, imageSize, 14), Qt::AlignCenter, label);
    } else {
        // Fallback to geometric representation
        paintGeometric(painter);
    }
}

void Component::paintGeometric(QPainter* painter)
{
    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(m_color);
    
    qreal halfSize = m_size / 2.0;
    
    switch (m_type) {
        case ComponentType::Antenna: {
            // Draw antenna with dish and support structure
            painter->drawEllipse(-halfSize, -halfSize * 0.6, m_size, m_size * 0.6);
            painter->drawLine(0, halfSize * 0.3, 0, halfSize);
            painter->drawLine(-halfSize * 0.4, halfSize, halfSize * 0.4, halfSize);
            // Draw label
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12), Qt::AlignCenter, "ANT");
            break;
        }
            
        case ComponentType::PowerSystem: {
            // Draw power system as battery/power unit
            painter->drawRect(-halfSize, -halfSize * 0.7, m_size, m_size * 1.4);
            painter->drawRect(-halfSize * 0.3, -halfSize * 0.9, m_size * 0.6, m_size * 0.2);
            // Draw power symbol
            QPen yellowPen(Qt::yellow, 3);
            painter->setPen(yellowPen);
            painter->drawLine(-halfSize * 0.3, -halfSize * 0.3, 0, 0);
            painter->drawLine(0, 0, halfSize * 0.3, halfSize * 0.3);
            painter->setPen(QPen(Qt::black, 2));
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12), Qt::AlignCenter, "PWR");
            break;
        }
            
        case ComponentType::LiquidCoolingUnit: {
            // Draw cooling unit with pipes
            painter->drawEllipse(-halfSize, -halfSize, m_size, m_size);
            painter->drawRect(-halfSize * 0.6, -halfSize * 1.2, m_size * 0.25, m_size * 0.25);
            painter->drawRect(halfSize * 0.35, -halfSize * 1.2, m_size * 0.25, m_size * 0.25);
            painter->drawLine(-halfSize * 0.5, -halfSize * 0.95, -halfSize * 0.5, -halfSize * 0.7);
            painter->drawLine(halfSize * 0.5, -halfSize * 0.95, halfSize * 0.5, -halfSize * 0.7);
            // Draw snowflake symbol
            QPen bluePen(Qt::cyan, 2);
            painter->setPen(bluePen);
            painter->drawLine(-halfSize * 0.3, 0, halfSize * 0.3, 0);
            painter->drawLine(0, -halfSize * 0.3, 0, halfSize * 0.3);
            painter->setPen(QPen(Qt::black, 2));
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12), Qt::AlignCenter, "COOL");
            break;
        }
            
        case ComponentType::CommunicationSystem: {
            // Draw communication system with signal waves
            painter->drawRect(-halfSize * 0.6, -halfSize * 0.6, m_size * 1.2, m_size * 1.2);
            // Draw signal waves
            for (int i = 1; i <= 3; i++) {
                qreal radius = halfSize * 0.3 * i;
                painter->drawArc(-radius, -radius, radius * 2, radius * 2, 45 * 16, 90 * 16);
            }
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12), Qt::AlignCenter, "COMM");
            break;
        }
            
        case ComponentType::RadarComputer: {
            // Draw computer/processor unit
            painter->drawRect(-halfSize, -halfSize * 0.8, m_size, m_size * 1.6);
            // Draw circuit pattern
            painter->drawLine(-halfSize * 0.5, -halfSize * 0.4, halfSize * 0.5, -halfSize * 0.4);
            painter->drawLine(-halfSize * 0.5, 0, halfSize * 0.5, 0);
            painter->drawLine(-halfSize * 0.5, halfSize * 0.4, halfSize * 0.5, halfSize * 0.4);
            painter->drawEllipse(-halfSize * 0.3, -halfSize * 0.2, m_size * 0.2, m_size * 0.2);
            painter->drawEllipse(halfSize * 0.1, -halfSize * 0.2, m_size * 0.2, m_size * 0.2);
            painter->setFont(QFont("Arial", 8));
            painter->drawText(QRectF(-halfSize, halfSize + 2, m_size, 12), Qt::AlignCenter, "CPU");
            break;
        }
    }
}

void Component::setColor(const QColor& color)
{
    qDebug() << "[Component]" << m_id << "- Color changing from" << m_color.name() << "to" << color.name();
    m_color = color;
    update();
    qDebug() << "[Component]" << m_id << "- Color updated, repaint triggered";
}

void Component::setSize(qreal size)
{
    qDebug() << "[Component]" << m_id << "- Size changing from" << m_size << "to" << size;
    prepareGeometryChange();
    m_size = size;
    update();
    qDebug() << "[Component]" << m_id << "- Size updated, repaint triggered";
}

QString Component::toJson() const
{
    QString typeStr;
    switch (m_type) {
        case ComponentType::Antenna: typeStr = "Antenna"; break;
        case ComponentType::PowerSystem: typeStr = "PowerSystem"; break;
        case ComponentType::LiquidCoolingUnit: typeStr = "LiquidCoolingUnit"; break;
        case ComponentType::CommunicationSystem: typeStr = "CommunicationSystem"; break;
        case ComponentType::RadarComputer: typeStr = "RadarComputer"; break;
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

void Component::loadSubsystemImage()
{
    QString dirName = getSubsystemDirName(m_type);
    QString imagePath = QString("assets/subsystems/%1/%2_main.jpg").arg(dirName).arg(dirName);
    
    // Check if file exists
    QFileInfo checkFile(imagePath);
    if (checkFile.exists() && checkFile.isFile()) {
        m_image.load(imagePath);
        if (!m_image.isNull()) {
            m_hasImage = true;
            // Scale image to a reasonable size for better performance
            if (m_image.width() > 512 || m_image.height() > 512) {
                m_image = m_image.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    } else {
        // Try PNG extension as fallback
        imagePath = QString("assets/subsystems/%1/%2_main.png").arg(dirName).arg(dirName);
        QFileInfo checkFilePng(imagePath);
        if (checkFilePng.exists() && checkFilePng.isFile()) {
            m_image.load(imagePath);
            if (!m_image.isNull()) {
                m_hasImage = true;
                // Scale image to a reasonable size for better performance
                if (m_image.width() > 512 || m_image.height() > 512) {
                    m_image = m_image.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
            }
        }
    }
}

QString Component::typeName(ComponentType type)
{
    switch (type) {
        case ComponentType::Antenna:             return "Antenna";
        case ComponentType::PowerSystem:         return "Power System";
        case ComponentType::LiquidCoolingUnit:   return "Cooling";
        case ComponentType::CommunicationSystem: return "Comms";
        case ComponentType::RadarComputer:       return "Radar Computer";
        default:                                 return "Unknown";
    }
}

QString Component::typeShortName(ComponentType type)
{
    switch (type) {
        case ComponentType::Antenna:             return "Antenna";
        case ComponentType::PowerSystem:         return "Power";
        case ComponentType::LiquidCoolingUnit:   return "Cooling";
        case ComponentType::CommunicationSystem: return "Comms";
        case ComponentType::RadarComputer:       return "Radar";
        default:                                 return "Unknown";
    }
}

QString Component::getSubsystemDirName(ComponentType type)
{
    switch (type) {
        case ComponentType::Antenna:
            return "antenna";
        case ComponentType::PowerSystem:
            return "power_system";
        case ComponentType::LiquidCoolingUnit:
            return "liquid_cooling_unit";
        case ComponentType::CommunicationSystem:
            return "communication_system";
        case ComponentType::RadarComputer:
            return "radar_computer";
        default:
            return "antenna";
    }
}
