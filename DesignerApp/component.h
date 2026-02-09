#ifndef COMPONENT_H
#define COMPONENT_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QPixmap>
#include <QList>
#include "subcomponent.h"

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
    
    // ------------------------------------------------------------------
    // Sub-component management
    // ------------------------------------------------------------------
    
    /** Returns the rectangle (in local coords) where subcomponents live. */
    QRectF containerRect() const;
    
    /** Whether this component currently hosts any subcomponents. */
    bool isInContainerMode() const { return !m_subComponents.isEmpty(); }
    
    /** Checks validation rules for placing a subcomponent type in this component. */
    bool canAcceptSubComponent(SubComponentType subType) const;
    
    /** Adds a subcomponent as a child item. Caller must set pos() after this. */
    void addSubComponent(SubComponent* sub);
    
    /** Removes and deletes a subcomponent. */
    void removeSubComponent(SubComponent* sub);
    
    /** Returns current child subcomponents. */
    QList<SubComponent*> getSubComponents() const { return m_subComponents; }
    
    /** Returns the set of allowed SubComponentTypes for a given ComponentType. */
    static QList<SubComponentType> allowedSubComponentTypes(ComponentType compType);
    
    /** Human-readable validation error message. Empty string if valid. */
    static QString validationMessage(ComponentType compType, SubComponentType subType);
    
    // ------------------------------------------------------------------
    // Serialization
    // ------------------------------------------------------------------
    
    QString toJson() const;
    static Component* fromJson(const QString& id, ComponentType type, qreal x, qreal y, 
                               const QColor& color, qreal size);
    
    // Helper method to get subsystem directory name
    static QString getSubsystemDirName(ComponentType type);
    static QString componentTypeName(ComponentType type);

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
    
    QList<SubComponent*> m_subComponents;
};

#endif // COMPONENT_H
