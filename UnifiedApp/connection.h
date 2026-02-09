#ifndef CONNECTION_H
#define CONNECTION_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>

class Component;

/**
 * ConnectionType - Defines the directionality of a connection.
 */
enum class ConnectionType {
    Unidirectional,   // Arrow from source to target
    Bidirectional     // Arrows in both directions
};

/**
 * Connection - A visual line/arrow connecting two Components on the canvas.
 * 
 * Supports:
 *   - Uni-directional arrows (A -> B)
 *   - Bi-directional arrows (A <-> B)
 *   - Text labels displayed along the connection line
 *   - Automatic position updates when connected components move
 */
class Connection : public QGraphicsItem
{
public:
    Connection(Component* source, Component* target, 
               ConnectionType type = ConnectionType::Unidirectional,
               const QString& label = QString(),
               QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QPainterPath shape() const override;
    
    // Accessors
    Component* getSource() const { return m_source; }
    Component* getTarget() const { return m_target; }
    ConnectionType getConnectionType() const { return m_connectionType; }
    QString getLabel() const { return m_label; }
    QString getId() const { return m_id; }
    QColor getColor() const { return m_color; }
    
    // Mutators
    void setLabel(const QString& label);
    void setConnectionType(ConnectionType type);
    void setColor(const QColor& color);
    void setId(const QString& id) { m_id = id; }
    
    // Update the line positions (called when components move)
    void updatePosition();
    
    // Check if this connection involves a given component
    bool involvesComponent(const Component* comp) const;
    
    // Serialization helpers
    static QString connectionTypeToString(ConnectionType type);
    static ConnectionType stringToConnectionType(const QString& str);

private:
    void drawArrowHead(QPainter* painter, const QPointF& tip, const QPointF& from, qreal size);
    QPointF getConnectionPoint(Component* comp, const QPointF& otherCenter) const;
    
    Component* m_source;
    Component* m_target;
    ConnectionType m_connectionType;
    QString m_label;
    QString m_id;
    QColor m_color;
    
    // Cached line endpoints
    QPointF m_sourcePoint;
    QPointF m_targetPoint;
};

#endif // CONNECTION_H
