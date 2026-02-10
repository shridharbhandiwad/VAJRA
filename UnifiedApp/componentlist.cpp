#include "componentlist.h"
#include "componentregistry.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QDebug>
#include <QMessageBox>

// ============================================================
// ComponentItemWidget Implementation
// ============================================================

ComponentItemWidget::ComponentItemWidget(const QString& displayName, const QString& typeId, QWidget* parent)
    : QWidget(parent)
    , m_displayName(displayName)
    , m_typeId(typeId)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);
    
    // Component name label
    m_nameLabel = new QLabel(displayName);
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_nameLabel->setStyleSheet(
        "QLabel {"
        "    color: #c4c7cc;"
        "    font-size: 13px;"
        "    font-weight: 500;"
        "    background: transparent;"
        "    border: none;"
        "}"
    );
    layout->addWidget(m_nameLabel);
    
    // Delete button (circular, with × symbol)
    m_deleteBtn = new QPushButton("×");
    m_deleteBtn->setFixedSize(24, 24);
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    m_deleteBtn->setStyleSheet(
        "QPushButton {"
        "    background-color: #ef5350;"
        "    color: white;"
        "    border: 1px solid #d32f2f;"
        "    border-radius: 12px;"
        "    font-size: 18px;"
        "    font-weight: bold;"
        "    padding: 0px;"
        "    margin: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #f44336;"
        "    border: 1px solid #c62828;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #c62828;"
        "    border: 1px solid #b71c1c;"
        "}"
    );
    m_deleteBtn->setToolTip(QString("Delete %1 component type").arg(displayName));
    layout->addWidget(m_deleteBtn);
    
    // Set minimum height to ensure visibility
    setMinimumHeight(32);
    
    // Set transparent background for the widget itself
    setStyleSheet(
        "ComponentItemWidget {"
        "    background: transparent;"
        "    border: none;"
        "}"
    );
    
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_typeId);
    });
}

// ============================================================
// ComponentList Implementation
// ============================================================

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
    
    QListWidgetItem* item = new QListWidgetItem();
    item->setData(Qt::UserRole, typeId);  // Store typeId for drag-drop
    item->setData(Qt::UserRole + 1, QStringLiteral("component")); // Item category
    item->setToolTip(typeId);
    addItem(item);
    
    // Create custom widget with delete button
    ComponentItemWidget* widget = new ComponentItemWidget(displayName, typeId);
    connect(widget, &ComponentItemWidget::deleteRequested, 
            this, &ComponentList::onDeleteComponent);
    
    // Ensure proper size hint with minimum height
    QSize hint = widget->sizeHint();
    hint.setHeight(qMax(hint.height(), 36));
    item->setSizeHint(hint);
    setItemWidget(item, widget);
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

void ComponentList::onDeleteComponent(const QString& typeId)
{
    ComponentRegistry& registry = ComponentRegistry::instance();
    ComponentDefinition def = registry.getComponent(typeId);
    
    if (def.typeId.isEmpty()) {
        qWarning() << "[ComponentList] Component type not found:" << typeId;
        return;
    }
    
    // Confirm deletion
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Component Type",
        QString("Are you sure you want to delete the component type '%1'?\n\n"
                "This will remove:\n"
                "• The component type definition\n"
                "• All instances of this component from the canvas\n\n"
                "This action cannot be undone.")
            .arg(def.displayName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    // Remove from registry (this will emit componentUnregistered and registryChanged signals)
    if (registry.unregisterComponent(typeId)) {
        qDebug() << "[ComponentList] Deleted component type:" << typeId;
        
        // Save registry to persist changes
        if (registry.saveToFile()) {
            qDebug() << "[ComponentList] Registry saved after deletion";
        } else {
            qWarning() << "[ComponentList] Failed to save registry after deletion";
        }
    } else {
        QMessageBox::warning(
            this,
            "Delete Failed",
            QString("Failed to delete component type '%1'").arg(def.displayName)
        );
    }
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
