#ifndef COMPONENTLIST_H
#define COMPONENTLIST_H

#include <QListWidget>
#include <QMouseEvent>
#include "component.h"

class ComponentList : public QListWidget
{
    Q_OBJECT
    
public:
    explicit ComponentList(QWidget* parent = nullptr);
    
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    
private:
    void addComponentItem(const QString& name, ComponentType type);
    QPoint m_dragStartPosition;
};

#endif // COMPONENTLIST_H
