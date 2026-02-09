#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QList>
#include <QMap>
#include "component.h"

/**
 * Canvas - The main design/runtime canvas for placing and viewing components.
 * 
 * MODULAR DESIGN: The canvas no longer knows about specific component types.
 * It uses the ComponentRegistry to resolve drag-drop type names to type IDs.
 * Any component type registered in the registry can be placed on the canvas.
 */
class Canvas : public QGraphicsView
{
    Q_OBJECT
    
public:
    explicit Canvas(QWidget* parent = nullptr);
    ~Canvas();
    
    void addComponent(const QString& typeId);
    QList<Component*> getComponents() const;
    Component* getComponentById(const QString& id);
    void clearCanvas();
    void loadFromJson(const QString& json);
    QString saveToJson() const;
    
signals:
    void componentAdded(const QString& id, const QString& typeId);
    void componentLoaded(const QString& id, const QString& typeId);
    
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    
private:
    QGraphicsScene* m_scene;
    int m_componentCounter;
    QMap<QString, Component*> m_componentMap;
};

#endif // CANVAS_H
