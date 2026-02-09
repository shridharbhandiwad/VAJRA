#ifndef COMPONENT_H
#define COMPONENT_H

#include <QGraphicsItem>
#include <QColor>
#include <QString>
#include <QPainter>
#include <QPixmap>

/**
 * Component - A visual graphics item representing a radar subsystem on the canvas.
 * 
 * MODULAR DESIGN: This class no longer uses a hardcoded enum for component types.
 * Instead, it uses a string-based type ID that maps to a ComponentDefinition
 * in the ComponentRegistry. New component types can be added via JSON config
 * or the UI without modifying this class.
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
    
    // Serialization
    QString toJson() const;
    static Component* fromJson(const QString& id, const QString& typeId, qreal x, qreal y, 
                               const QColor& color, qreal size);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void loadSubsystemImage();
    void paintWithImage(QPainter* painter);
    void paintGeometric(QPainter* painter);
    void drawShape(QPainter* painter, const QString& shape, qreal halfSize);
    
    QString m_typeId;
    QString m_id;
    QColor m_color;
    qreal m_size;
    QPixmap m_image;
    bool m_hasImage;
};

#endif // COMPONENT_H
