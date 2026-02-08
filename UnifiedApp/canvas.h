#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QList>
#include <QMap>
#include "component.h"

class Canvas : public QGraphicsView
{
    Q_OBJECT
    
public:
    explicit Canvas(QWidget* parent = nullptr);
    ~Canvas();
    
    void addComponent(ComponentType type);
    QList<Component*> getComponents() const;
    Component* getComponentById(const QString& id);
    void clearCanvas();
    void loadFromJson(const QString& json);
    QString saveToJson() const;
    
signals:
    void componentAdded(const QString& id, ComponentType type);
    void componentLoaded(const QString& id, const QString& type);
    
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
