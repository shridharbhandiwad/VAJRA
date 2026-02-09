#include "componentlist.h"
#include "componentregistry.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QFileInfo>
#include <QPainter>
#include <QDebug>

ComponentList::ComponentList(QWidget* parent)
    : QListWidget(parent)
{
    setIconSize(QSize(48, 48));
    setDragEnabled(true);
    setMaximumWidth(220);
    setSpacing(2);
    
    // Populate from registry
    refreshFromRegistry();
    
    // Auto-refresh when registry changes
    connect(&ComponentRegistry::instance(), &ComponentRegistry::registryChanged,
            this, &ComponentList::refreshFromRegistry);
}

void ComponentList::refreshFromRegistry()
{
    clear();
    
    ComponentRegistry& registry = ComponentRegistry::instance();
    QList<ComponentDefinition> components = registry.getAllComponents();
    
    for (const ComponentDefinition& def : components) {
        addComponentItem(def.displayName, def.typeId, def.imageDir, def.iconColor);
    }
    
    qDebug() << "[ComponentList] Refreshed with" << components.size() << "component types";
}

void ComponentList::addComponentItem(const QString& displayName, const QString& typeId, 
                                      const QString& imageDir, const QColor& iconColor)
{
    QListWidgetItem* item = new QListWidgetItem(displayName);
    item->setData(Qt::UserRole, typeId);  // Store typeId for drag-drop
    item->setToolTip(typeId);
    
    bool iconSet = false;
    
    // Try to load icon from image directory
    if (!imageDir.isEmpty()) {
        // Try JPG first
        QString iconPath = QString("assets/subsystems/%1/%1_main.jpg").arg(imageDir);
        QFileInfo checkFile(iconPath);
        if (checkFile.exists() && checkFile.isFile()) {
            QPixmap pixmap(iconPath);
            if (!pixmap.isNull()) {
                QPixmap scaledPixmap = pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                item->setIcon(QIcon(scaledPixmap));
                iconSet = true;
            }
        }
        
        if (!iconSet) {
            // Try PNG
            iconPath = QString("assets/subsystems/%1/%1_main.png").arg(imageDir);
            QFileInfo checkFilePng(iconPath);
            if (checkFilePng.exists() && checkFilePng.isFile()) {
                QPixmap pixmap(iconPath);
                if (!pixmap.isNull()) {
                    QPixmap scaledPixmap = pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    item->setIcon(QIcon(scaledPixmap));
                    iconSet = true;
                }
            }
        }
    }
    
    // Generate a colored icon if no image is available
    if (!iconSet) {
        QPixmap colorIcon(48, 48);
        colorIcon.fill(Qt::transparent);
        
        QPainter painter(&colorIcon);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Draw a colored circle with the label
        painter.setPen(Qt::NoPen);
        painter.setBrush(iconColor);
        painter.drawRoundedRect(4, 4, 40, 40, 8, 8);
        
        // Draw label text
        painter.setPen(Qt::white);
        painter.setFont(QFont("Segoe UI", 10, QFont::Bold));
        ComponentRegistry& registry = ComponentRegistry::instance();
        QString label = registry.hasComponent(typeId) ? registry.getComponent(typeId).label : typeId.left(3);
        painter.drawText(QRect(4, 4, 40, 40), Qt::AlignCenter, label);
        painter.end();
        
        item->setIcon(QIcon(colorIcon));
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
    
    // Send display name as text (for backward compat) and typeId in custom format
    mimeData->setText(item->text());
    mimeData->setData("application/x-component-typeid", 
                       item->data(Qt::UserRole).toString().toUtf8());
    drag->setMimeData(mimeData);
    
    drag->exec(Qt::CopyAction);
}
