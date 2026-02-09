#ifndef SUBCOMPONENT_H
#define SUBCOMPONENT_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>

/**
 * SubComponent - A small visual item embedded inside a parent Component.
 * 
 * Represents a sub-system within a larger system component (e.g., "Signal Strength"
 * inside "Antenna"). Sub-components are rendered within the parent's bounding rect
 * and display their own health status.
 */
class SubComponent : public QGraphicsItem
{
public:
    SubComponent(const QString& name, int index, QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    // Accessors
    QString getName() const { return m_name; }
    QColor getColor() const { return m_color; }
    qreal getHealth() const { return m_health; }
    int getIndex() const { return m_index; }
    
    // Mutators
    void setColor(const QColor& color);
    void setHealth(qreal health);
    void setIndex(int index);
    
    // Geometry
    static qreal itemWidth() { return 130; }
    static qreal itemHeight() { return 28; }
    
    // Get center point in parent coordinates for connection drawing
    QPointF centerInParent() const;

private:
    QString m_name;
    QColor m_color;
    qreal m_health;   // 0-100
    int m_index;       // Position index within parent
};

#endif // SUBCOMPONENT_H
