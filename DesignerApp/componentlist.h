#ifndef COMPONENTLIST_H
#define COMPONENTLIST_H

#include <QListWidget>
#include <QMouseEvent>

class ComponentList : public QListWidget
{
    Q_OBJECT
    
public:
    explicit ComponentList(QWidget* parent = nullptr);
    
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    
private:
    QPoint m_dragStartPosition;
};

#endif // COMPONENTLIST_H
