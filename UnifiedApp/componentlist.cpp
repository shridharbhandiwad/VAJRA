#include "componentlist.h"
#include "componentregistry.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QFileInfo>
#include <QPainter>
#include <QLinearGradient>
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
    
    // --- Separator ---
    QListWidgetItem* separator = new QListWidgetItem("  ─── Sub-Components ───");
    separator->setFlags(Qt::NoItemFlags);   // not selectable / draggable
    separator->setBackground(QColor(36, 39, 46));
    separator->setForeground(QColor(120, 125, 135));
    QFont sepFont("Segoe UI", 8, QFont::Bold);
    separator->setFont(sepFont);
    addItem(separator);
    
    // --- Draggable sub-component widget types ---
    addSubComponentItem("Label");
    addSubComponentItem("LineEdit");
    addSubComponentItem("Button");
    
    qDebug() << "[ComponentList] Refreshed with" << components.size() << "component types + 3 sub-component types";
}

void ComponentList::addComponentItem(const QString& displayName, const QString& typeId, 
                                      const QString& imageDir, const QColor& iconColor)
{
    QListWidgetItem* item = new QListWidgetItem(displayName);
    item->setData(Qt::UserRole, typeId);  // Store typeId for drag-drop
    item->setData(Qt::UserRole + 1, QStringLiteral("component")); // Item category
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

void ComponentList::addSubComponentItem(const QString& name)
{
    QListWidgetItem* item = new QListWidgetItem(name);
    
    // Mark as a sub-component item
    item->setData(Qt::UserRole, name);  // Store the type name
    item->setData(Qt::UserRole + 1, QStringLiteral("subcomponent")); // Item category
    item->setToolTip(QString("Drag and drop '%1' widget into a component").arg(name));
    
    // Generate a small coloured icon for each sub-component type
    QPixmap pix(48, 48);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    
    if (name == "Label") {
        p.setPen(QPen(QColor(100, 105, 115), 1));
        p.setBrush(QColor(52, 56, 63));
        p.drawRoundedRect(4, 12, 40, 24, 3, 3);
        p.setPen(QColor(220, 222, 228));
        p.setFont(QFont("Segoe UI", 8));
        p.drawText(QRect(4, 12, 40, 24), Qt::AlignCenter, "Abc");
    } else if (name == "LineEdit") {
        p.setPen(QPen(QColor(90, 95, 105), 1));
        p.setBrush(QColor(40, 42, 50));
        p.drawRoundedRect(4, 12, 40, 24, 3, 3);
        p.setPen(QPen(QColor(0, 188, 212), 2));
        p.drawLine(6, 34, 42, 34);
        p.setPen(QColor(140, 145, 155));
        p.setFont(QFont("Segoe UI", 7));
        p.drawText(QRect(4, 12, 40, 22), Qt::AlignCenter, "Edit");
    } else if (name == "Button") {
        QLinearGradient grad(0, 12, 0, 36);
        grad.setColorAt(0, QColor(0, 188, 212));
        grad.setColorAt(1, QColor(0, 151, 167));
        p.setPen(QPen(QColor(0, 131, 143), 1));
        p.setBrush(grad);
        p.drawRoundedRect(4, 12, 40, 24, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 7, QFont::Bold));
        p.drawText(QRect(4, 12, 40, 24), Qt::AlignCenter, "Btn");
    }
    p.end();
    
    item->setIcon(QIcon(pix));
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
    
    // Use itemAt to get the item where the drag started (avoids dragging the separator)
    QListWidgetItem* item = itemAt(m_dragStartPosition);
    if (!item || !(item->flags() & Qt::ItemIsEnabled)) {
        return;
    }
    
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    
    QString category = item->data(Qt::UserRole + 1).toString();
    
    if (category == "subcomponent") {
        // Sub-component: use "subcomponent:TypeName" format
        mimeData->setText("subcomponent:" + item->data(Qt::UserRole).toString());
    } else {
        // Component: send display name as text and typeId in custom format
        mimeData->setText(item->text());
        mimeData->setData("application/x-component-typeid", 
                           item->data(Qt::UserRole).toString().toUtf8());
    }
    
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}
