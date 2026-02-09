#ifndef COMPONENT_H
#define COMPONENT_H

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QPixmap>
#include <QList>
#include "subcomponent.h"
#include "designsubcomponent.h"

/**
 * Component - A visual graphics item representing a radar subsystem on the canvas.
 * 
 * MODULAR DESIGN: This class no longer uses a hardcoded enum for component types.
 * Instead, it uses a string-based type ID that maps to a ComponentDefinition
 * in the ComponentRegistry. New component types can be added via JSON config
 * or the UI without modifying this class.
 * 
 * HIERARCHY: Each component can contain SubComponent items embedded within it.
 * Sub-components are created from the "subsystems" field of the ComponentDefinition.
 * 
 * RELATIONS: Components can be connected via Connection objects that draw
 * uni-directional or bi-directional arrows with labels.
 * 
 * RESIZING: Components can be resized by the user via corner and edge handles
 * that appear when the component is selected. This allows the user to increase
 * the component window size so more subcomponents can be added.
 */
class Component : public QGraphicsItem
{
public:
    Component(const QString& typeId, const QString& id, QGraphicsItem* parent = nullptr);
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    // Accessors
    QString getTypeId() const { return m_typeId; }
    QString getId() const { return m_id; }
    QColor getColor() const { return m_color; }
    qreal getSize() const { return m_size; }
    QString getDisplayName() const;
    QString getLabel() const;
    
    // Mutators
    void setColor(const QColor& color);
    void setSize(qreal size);
    
    // User-resizable dimensions
    void setUserWidth(qreal w);
    void setUserHeight(qreal h);
    qreal getUserWidth() const { return m_userWidth; }
    qreal getUserHeight() const { return m_userHeight; }
    
    // Sub-component management (auto-created health-tracking subsystems)
    void addSubComponent(const QString& name);
    void removeSubComponent(int index);
    QList<SubComponent*> getSubComponents() const { return m_subComponents; }
    SubComponent* getSubComponent(const QString& name) const;
    int subComponentCount() const { return m_subComponents.size(); }
    
    // Design sub-component management (user drag-dropped Label/LineEdit/Button widgets)
    QRectF designContainerRect() const;
    bool canAcceptDesignSubComponent(SubComponentType subType) const;
    void addDesignSubComponent(DesignSubComponent* sub);
    void removeDesignSubComponent(DesignSubComponent* sub);
    QList<DesignSubComponent*> getDesignSubComponents() const { return m_designSubComponents; }
    int designSubComponentCount() const { return m_designSubComponents.size(); }
    
    /** Returns the set of allowed SubComponentTypes for this component's type. */
    static QStringList allowedWidgetTypes(const QString& typeId);
    
    /** Human-readable validation error message. Empty string if valid. */
    static QString widgetValidationMessage(const QString& typeId, SubComponentType subType);
    
    // Container dimensions
    qreal containerWidth() const;
    qreal containerHeight() const;
    
    // Connection anchor point (center of the component)
    QPointF anchorPoint() const;
    
    // Serialization
    QString toJson() const;
    static Component* fromJson(const QString& id, const QString& typeId, qreal x, qreal y, 
                               const QColor& color, qreal size);

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
    
    void loadSubsystemImage();
    void createDefaultSubComponents();
    void layoutSubComponents();
    void paintContainer(QPainter* painter);
    
    QString m_typeId;
    QString m_id;
    QColor m_color;
    qreal m_size;
    QPixmap m_image;
    bool m_hasImage;
    
    // User-resizable dimensions (0 = use auto-computed size)
    qreal m_userWidth;
    qreal m_userHeight;
    
    // Resize state
    ResizeHandle m_activeHandle;
    QPointF m_lastMouseScenePos;
    bool m_resizing;
    
    QList<SubComponent*> m_subComponents;
    QList<DesignSubComponent*> m_designSubComponents;
    
    // Layout constants
    static constexpr qreal HEADER_HEIGHT = 40;
    static constexpr qreal PADDING = 8;
    static constexpr qreal SUB_SPACING = 4;
    static constexpr qreal MIN_WIDTH = 160;
    static constexpr qreal FOOTER_HEIGHT = 8;
    static constexpr qreal DESIGN_CONTAINER_HEADER = 18.0;
    static constexpr qreal DESIGN_CONTAINER_MIN_HEIGHT = 35.0;
    static constexpr qreal DESIGN_CONTAINER_FULL_HEIGHT = 150.0;
    static constexpr qreal RESIZE_HANDLE_SIZE = 8.0;
};

#endif // COMPONENT_H
