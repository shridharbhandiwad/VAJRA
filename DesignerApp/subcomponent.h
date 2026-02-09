#ifndef DESIGNERSUBCOMPONENT_H
#define DESIGNERSUBCOMPONENT_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>

/**
 * SubComponentType - Types of subcomponents that can be placed inside parent components.
 *
 * Validation rules (which types are allowed per parent component):
 *   Antenna:              Label, Button
 *   PowerSystem:          Label, LineEdit, Button
 *   LiquidCoolingUnit:    Label, Button
 *   CommunicationSystem:  Label, LineEdit, Button
 *   RadarComputer:        Label, LineEdit, Button
 */
enum class SubComponentType {
    Label,
    LineEdit,
    Button
};

/**
 * SubComponent - A draggable, resizable visual item placed inside a parent Component.
 *
 * Features:
 *   - Three visual types: Label (static text), LineEdit (input field), Button
 *   - Four corner resize handles (visible when selected)
 *   - Movement constrained to parent Component's container area
 *   - Serializable to/from JSON for save/load
 */
class SubComponent : public QGraphicsItem
{
public:
    SubComponent(SubComponentType type, const QString& text, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    // Accessors
    SubComponentType getType() const { return m_type; }
    QString getText() const { return m_text; }
    qreal getWidth() const { return m_width; }
    qreal getHeight() const { return m_height; }

    // Mutators
    void setText(const QString& text);
    void setSize(qreal w, qreal h);

    // Serialization helpers
    static QString typeToString(SubComponentType type);
    static SubComponentType stringToType(const QString& str);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    enum ResizeHandle {
        HandleNone = 0,
        HandleTopLeft,
        HandleTopRight,
        HandleBottomLeft,
        HandleBottomRight
    };

    ResizeHandle handleAt(const QPointF& pos) const;
    void paintLabel(QPainter* painter);
    void paintLineEdit(QPainter* painter);
    void paintButton(QPainter* painter);
    void paintResizeHandles(QPainter* painter);

    SubComponentType m_type;
    QString m_text;
    qreal m_width;
    qreal m_height;
    ResizeHandle m_activeHandle;
    QPointF m_lastMouseScenePos;
    bool m_resizing;

    static constexpr qreal HANDLE_SIZE = 6.0;
    static constexpr qreal MIN_WIDTH = 40.0;
    static constexpr qreal MIN_HEIGHT = 20.0;
};

#endif // DESIGNERSUBCOMPONENT_H
