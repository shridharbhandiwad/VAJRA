#include "componentlist.h"
#include "component.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QFileInfo>

ComponentList::ComponentList(QWidget* parent)
    : QListWidget(parent)
{
    // Set larger icon size for better visibility
    setIconSize(QSize(48, 48));
    
    // Add components with icons
    addComponentItem("Antenna", ComponentType::Antenna);
    addComponentItem("Power System", ComponentType::PowerSystem);
    addComponentItem("Liquid Cooling Unit", ComponentType::LiquidCoolingUnit);
    addComponentItem("Communication System", ComponentType::CommunicationSystem);
    addComponentItem("Radar Computer", ComponentType::RadarComputer);
    
    setDragEnabled(true);
    setMaximumWidth(180);
}

void ComponentList::addComponentItem(const QString& name, ComponentType type)
{
    QListWidgetItem* item = new QListWidgetItem(name);
    
    // Load icon for the component
    QString dirName = Component::getSubsystemDirName(type);
    QString iconPath = QString("assets/subsystems/%1/%2_main.jpg").arg(dirName).arg(dirName);
    
    QFileInfo checkFile(iconPath);
    if (checkFile.exists() && checkFile.isFile()) {
        QPixmap pixmap(iconPath);
        if (!pixmap.isNull()) {
            // Scale to icon size
            QPixmap scaledPixmap = pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            item->setIcon(QIcon(scaledPixmap));
        }
    } else {
        // Try PNG extension as fallback
        iconPath = QString("assets/subsystems/%1/%2_main.png").arg(dirName).arg(dirName);
        QFileInfo checkFilePng(iconPath);
        if (checkFilePng.exists() && checkFilePng.isFile()) {
            QPixmap pixmap(iconPath);
            if (!pixmap.isNull()) {
                // Scale to icon size
                QPixmap scaledPixmap = pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                item->setIcon(QIcon(scaledPixmap));
            }
        }
    }
    
    addItem(item);
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
