#include "componentlist.h"
#include "componentregistry.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QDebug>

ComponentList::ComponentList(QWidget* parent)
    : QListWidget(parent)
{
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
    QFont sepFont("Inter", 10, QFont::Bold);
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
    Q_UNUSED(imageDir);
    Q_UNUSED(iconColor);
    
    QListWidgetItem* item = new QListWidgetItem(displayName);
    item->setData(Qt::UserRole, typeId);  // Store typeId for drag-drop
    item->setData(Qt::UserRole + 1, QStringLiteral("component")); // Item category
    item->setToolTip(typeId);
    
    addItem(item);
}

void ComponentList::addSubComponentItem(const QString& name)
{
    QListWidgetItem* item = new QListWidgetItem(name);
    
    // Mark as a sub-component item
    item->setData(Qt::UserRole, name);  // Store the type name
    item->setData(Qt::UserRole + 1, QStringLiteral("subcomponent")); // Item category
    item->setToolTip(QString("Drag and drop '%1' widget into a component").arg(name));
    
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
