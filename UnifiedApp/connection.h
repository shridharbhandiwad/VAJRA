#ifndef CONNECTION_H
#define CONNECTION_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>

class Component;
class SubComponent;

/**
 * ConnectionType - Defines the directionality of a connection.
 */
enum class ConnectionType {
    Unidirectional,   // Arrow from source to target
    Bidirectional     // Arrows in both directions
};

/**
 * Connection - A visual line/arrow connecting two items (Components or SubComponents) on the canvas.
 * 
 * Supports:
 *   - Uni-directional arrows (A -> B)
 *   - Bi-directional arrows (A <-> B)
 *   - Text labels displayed along the connection line
 *   - Automatic position updates when connected items move
 *   - Connections between Components, SubComponents, or a mix of both
 */
class Connection : public QGraphicsItem
{
public:
    // Constructor for Component-to-Component connections
    Connection(Component* source, Component* target, 
               ConnectionType type = ConnectionType::Unidirectional,
               const QString& label = QString(),
               QGraphicsItem* parent = nullptr);
    
    // Constructor for generic item-to-item connections (supports SubComponents)
    Connection(QGraphicsItem* source, QGraphicsItem* target,
               ConnectionType type = ConnectionType::Unidirectional,
               const QString& label = QString(),
               QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QPainterPath shape() const override;
    
    // Accessors
    QGraphicsItem* getSourceItem() const { return m_sourceItem; }
    QGraphicsItem* getTargetItem() const { return m_targetItem; }
    Component* getSource() const;  // Returns Component if source is a Component, nullptr otherwise
    Component* getTarget() const;  // Returns Component if target is a Component, nullptr otherwise
    SubComponent* getSourceSub() const;  // Returns SubComponent if source is a SubComponent, nullptr otherwise
    SubComponent* getTargetSub() const;  // Returns SubComponent if target is a SubComponent, nullptr otherwise
    ConnectionType getConnectionType() const { return m_connectionType; }
    QString getLabel() const { return m_label; }
    QString getId() const { return m_id; }
    QColor getColor() const { return m_color; }
    
    // Mutators
    void setLabel(const QString& label);
    void setConnectionType(ConnectionType type);
    void setColor(const QColor& color);
    void setId(const QString& id) { m_id = id; }
    
    // Update the line positions (called when items move)
    void updatePosition();
    
    // Check if this connection involves a given component or subcomponent
    bool involvesComponent(const Component* comp) const;
    bool involvesSubComponent(const SubComponent* sub) const;
    bool involvesItem(const QGraphicsItem* item) const;
    
    // Serialization helpers
    static QString connectionTypeToString(ConnectionType type);
    static ConnectionType stringToConnectionType(const QString& str);

private:
    void drawArrowHead(QPainter* painter, const QPointF& tip, const QPointF& from, qreal size);
    QPointF getConnectionPoint(QGraphicsItem* item, const QPointF& otherCenter) const;
    QPointF getItemCenter(QGraphicsItem* item) const;
    
    QGraphicsItem* m_sourceItem;
    QGraphicsItem* m_targetItem;
    ConnectionType m_connectionType;
    QString m_label;
    QString m_id;
    QColor m_color;
    
    // Cached line endpoints
    QPointF m_sourcePoint;
    QPointF m_targetPoint;
};

#endif // CONNECTION_H
