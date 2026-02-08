#include "componentlist.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>

ComponentList::ComponentList(QWidget* parent)
    : QListWidget(parent)
{
    addItem("Antenna");
    addItem("Power System");
    addItem("Liquid Cooling Unit");
    addItem("Communication System");
    addItem("Radar Computer");
    
    setDragEnabled(true);
    setMaximumWidth(180);
}

void ComponentList::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }
    QListWidget::mousePressEvent(event);
}

void ComponentList::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }
    
    QListWidgetItem* item = currentItem();
    if (!item) {
        return;
    }
    
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    mimeData->setText(item->text());
    drag->setMimeData(mimeData);
    
    drag->exec(Qt::CopyAction);
}
