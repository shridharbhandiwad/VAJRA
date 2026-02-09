#ifndef DESIGNSUBCOMPONENT_H
#define DESIGNSUBCOMPONENT_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneContextMenuEvent>

/**
 * SubComponentType - Types of UI widget sub-components that can be
 * drag-dropped inside parent components.
 *
 * Validation rules (which types are allowed) are configured per
 * component type in components.json via the "allowed_widgets" field.
 *
 *   Default allowed_widgets:
 *     Antenna:              Label, Button
 *     PowerSystem:          Label, LineEdit, Button
 *     LiquidCoolingUnit:    Label, Button
 *     CommunicationSystem:  Label, LineEdit, Button
 *     RadarComputer:        Label, LineEdit, Button
 */
enum class SubComponentType {
    Label,
    LineEdit,
    Button
};

class Component;

/**
 * DesignSubComponent - A draggable, resizable visual widget placed inside
 * a parent Component on the design canvas.
 *
 * Features:
 *   - Three visual types: Label (static text), LineEdit (input field), Button
 *   - Four corner resize handles (visible when selected)
 *   - Movement constrained to parent Component's design container area
 *   - Double-click to edit text; right-click context menu for edit/delete
 *   - Health color and value properties for later health visualization
 *   - Serializable to/from JSON for save/load
 *   - Theme-aware rendering
 */
class DesignSubComponent : public QGraphicsItem
{
public:
    DesignSubComponent(SubComponentType type, const QString& text, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

    // Accessors
    SubComponentType getType() const { return m_type; }
    QString getText() const { return m_text; }
    qreal getWidth() const { return m_width; }
    qreal getHeight() const { return m_height; }
    QColor getHealthColor() const { return m_healthColor; }
    qreal getHealthValue() const { return m_healthValue; }

    // Mutators
    void setText(const QString& text);
    void setSize(qreal w, qreal h);
    void setHealthColor(const QColor& color);
    void setHealthValue(qreal value);

    // Serialization helpers
    static QString typeToString(SubComponentType type);
    static SubComponentType stringToType(const QString& str);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
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
    void paintHealthIndicator(QPainter* painter);
    void showTextEditDialog();

    SubComponentType m_type;
    QString m_text;
    qreal m_width;
    qreal m_height;
    ResizeHandle m_activeHandle;
    QPointF m_lastMouseScenePos;
    bool m_resizing;

    // Health properties (used later for health visualization)
    QColor m_healthColor;
    qreal m_healthValue;  // 0-100

    static constexpr qreal HANDLE_SIZE = 6.0;
    static constexpr qreal MIN_WIDTH = 40.0;
    static constexpr qreal MIN_HEIGHT = 20.0;
    static constexpr qreal HEALTH_BAR_WIDTH = 4.0;
};

#endif // DESIGNSUBCOMPONENT_H
