#ifndef SUBCOMPONENT_H
#define SUBCOMPONENT_H

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QColor>
#include <QString>
#include <QPainter>

class Component;

/**
 * SubComponent - A draggable, resizable visual item embedded inside a parent Component.
 * 
 * Represents a sub-system within a larger system component (e.g., "Signal Strength"
 * inside "Antenna"). Sub-components can be:
 *   - Freely positioned anywhere within their parent Component
 *   - Resized by the user via corner and edge handles
 *   - Connected to other SubComponents or Components via Connection objects
 *   - Display their own health status and properties
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
    qreal getWidth() const { return m_width; }
    qreal getHeight() const { return m_height; }
    
    // Mutators
    void setColor(const QColor& color);
    void setHealth(qreal health);
    void setIndex(int index);
    void setWidth(qreal w);
    void setHeight(qreal h);
    
    // Geometry (default sizes, but now user-resizable)
    static qreal defaultWidth() { return 130; }
    static qreal defaultHeight() { return 28; }
    static qreal minWidth() { return 80; }
    static qreal minHeight() { return 24; }
    
    // Get center point in parent coordinates for connection drawing
    QPointF centerInParent() const;
    
    // Get center point in scene coordinates for connection drawing
    QPointF centerInScene() const;
    
    // Get parent Component (for boundary checking)
    Component* parentComponent() const;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    // Resize handle enumeration
    enum ResizeHandle {
        HandleNone = 0,
        HandleTopLeft,
        HandleTop,
        HandleTopRight,
        HandleRight,
        HandleBottomRight,
        HandleBottom,
        HandleBottomLeft,
        HandleLeft
    };
    
    ResizeHandle handleAt(const QPointF& pos) const;
    void paintResizeHandles(QPainter* painter);
    QPointF constrainToBoundary(const QPointF& proposedPos) const;
    
    QString m_name;
    QColor m_color;
    qreal m_health;   // 0-100
    int m_index;       // Position index within parent
    
    // User-resizable dimensions
    qreal m_width;
    qreal m_height;
    
    // Resize state
    ResizeHandle m_activeHandle;
    QPointF m_lastMouseScenePos;
    bool m_resizing;
    bool m_dragging;
    
    static constexpr qreal RESIZE_HANDLE_SIZE = 6.0;
};

#endif // SUBCOMPONENT_H
