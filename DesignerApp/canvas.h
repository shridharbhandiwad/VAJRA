#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QList>
#include "component.h"
#include "subcomponent.h"

class Canvas : public QGraphicsView
{
    Q_OBJECT
    
public:
    explicit Canvas(QWidget* parent = nullptr);
    ~Canvas();
    
    void addComponent(ComponentType type);
    QList<Component*> getComponents() const;
    void clearCanvas();
    void loadFromJson(const QString& json);
    QString saveToJson() const;
    
signals:
    void componentAdded(const QString& id, ComponentType type);
    void subComponentAdded(const QString& parentId, SubComponentType subType);
    void dropRejected(const QString& reason);
    
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    Component* componentAtScenePos(const QPointF& scenePos) const;
    
    QGraphicsScene* m_scene;
    int m_componentCounter;
};

#endif // CANVAS_H
