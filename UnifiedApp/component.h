#ifndef COMPONENT_H
#define COMPONENT_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QPixmap>

enum class ComponentType {
    Antenna,
    PowerSystem,
    LiquidCoolingUnit,
    CommunicationSystem,
    RadarComputer
};

class Component : public QGraphicsItem
{
public:
    Component(ComponentType type, const QString& id, QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    ComponentType getType() const { return m_type; }
    QString getId() const { return m_id; }
    QColor getColor() const { return m_color; }
    qreal getSize() const { return m_size; }
    
    void setColor(const QColor& color);
    void setSize(qreal size);
    
    QString toJson() const;
    static Component* fromJson(const QString& id, ComponentType type, qreal x, qreal y, 
                               const QColor& color, qreal size);
    
    // Helper method to get subsystem directory name
    static QString getSubsystemDirName(ComponentType type);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void loadSubsystemImage();
    void paintGeometric(QPainter* painter);
    
    ComponentType m_type;
    QString m_id;
    QColor m_color;
    qreal m_size;
    QPixmap m_image;
    bool m_hasImage;
};

#endif // COMPONENT_H
