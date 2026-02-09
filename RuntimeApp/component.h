#ifndef COMPONENT_H
#define COMPONENT_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QPixmap>
#include <QList>

enum class ComponentType {
    Antenna,
    PowerSystem,
    LiquidCoolingUnit,
    CommunicationSystem,
    RadarComputer
};

struct SubcomponentInfo {
    QString name;
    int health;
    QColor color;
    SubcomponentInfo() : health(0), color(Qt::gray) {}
    SubcomponentInfo(const QString& n, int h, const QColor& c) : name(n), health(h), color(c) {}
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
    
    // Subcomponent management
    QList<SubcomponentInfo> getSubcomponents() const { return m_subcomponents; }
    void setSubcomponents(const QList<SubcomponentInfo>& subs) { m_subcomponents = subs; }
    
    // Human-readable name for component type
    static QString typeName(ComponentType type);
    static QString typeShortName(ComponentType type);
    
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
    QList<SubcomponentInfo> m_subcomponents;
};

#endif // COMPONENT_H
