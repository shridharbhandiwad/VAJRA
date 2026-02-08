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
    
    QList<Component*> getComponents() const;
    Component* getComponentById(const QString& id);
    void clearCanvas();
    void loadFromJson(const QString& json);
    
signals:
    void componentLoaded(const QString& id, const QString& type);
    
private:
    QGraphicsScene* m_scene;
    QMap<QString, Component*> m_componentMap;
};

#endif // CANVAS_H
