#include "canvas.h"
#include "componentregistry.h"
#include "thememanager.h"
#include "subcomponent.h"
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QInputDialog>
#include <QFile>

Canvas::Canvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_readOnly(false)
    , m_userRole(UserRole::Designer)
    , m_componentCounter(0)
    , m_connectionCounter(0)
    , m_mode(CanvasMode::Select)
    , m_pendingConnectionType(ConnectionType::Unidirectional)
    , m_connectionSourceItem(nullptr)
    , m_pendingLine(nullptr)
    , m_isDrawingConnection(false)
{
    setScene(m_scene);
    setAcceptDrops(true);
    setRenderHint(QPainter::Antialiasing);
    
    // Set scene size
    m_scene->setSceneRect(0, 0, 1200, 900);
    
    // Theme-aware background
    setBackgroundBrush(QBrush(ThemeManager::instance().canvasBackground()));
    
    // Enable rubber band selection in select mode
    setDragMode(QGraphicsView::NoDrag);
    
    // Connect to registry to handle component type deletion
    connect(&ComponentRegistry::instance(), &ComponentRegistry::componentUnregistered,
            this, &Canvas::removeComponentsByType);
}

Canvas::~Canvas()
{
}

void Canvas::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    setAcceptDrops(!readOnly);
    
    // In read-only mode, components should not be movable or selectable
    for (auto it = m_componentMap.begin(); it != m_componentMap.end(); ++it) {
        Component* comp = it.value();
        if (comp) {
            comp->setFlag(QGraphicsItem::ItemIsMovable, !readOnly);
            comp->setFlag(QGraphicsItem::ItemIsSelectable, !readOnly);
        }
    }
}

Component* Canvas::getComponentById(const QString& id)
{
    Component* comp = m_componentMap.value(id, nullptr);
    qDebug() << "[Canvas] getComponentById(" << id << ") =" << (comp ? "FOUND" : "NOT FOUND");
    return comp;
}

void Canvas::addComponent(const QString& typeId)
{
    // Verify type exists in registry
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (!registry.hasComponent(typeId)) {
        qWarning() << "[Canvas] Unknown component type:" << typeId;
        return;
    }
    
    QString id = QString("component_%1").arg(++m_componentCounter);
    Component* comp = new Component(typeId, id);
    
    // Place at center of view
    QPointF centerPos = mapToScene(viewport()->rect().center());
    comp->setPos(centerPos);
    
    m_scene->addItem(comp);
    m_componentMap[id] = comp;
    emit componentAdded(id, typeId);
}

QList<Component*> Canvas::getComponents() const
{
    QList<Component*> components;
    foreach (QGraphicsItem* item, m_scene->items()) {
        if (Component* comp = dynamic_cast<Component*>(item)) {
            components.append(comp);
        }
    }
    return components;
}

void Canvas::clearCanvas()
{
    // Remove connections first
    for (Connection* conn : m_connections) {
        m_scene->removeItem(conn);
        delete conn;
    }
    m_connections.clear();
    
    m_scene->clear();
    m_componentCounter = 0;
    m_connectionCounter = 0;
    m_componentMap.clear();
    m_connectionSourceItem = nullptr;
    m_pendingLine = nullptr;
    m_isDrawingConnection = false;
}

void Canvas::removeComponentsByType(const QString& typeId)
{
    qDebug() << "[Canvas] Removing all components of type:" << typeId;
    
    // Find all components with this type
    QList<Component*> componentsToRemove;
    for (auto it = m_componentMap.begin(); it != m_componentMap.end(); ++it) {
        Component* comp = it.value();
        if (comp && comp->getTypeId() == typeId) {
            componentsToRemove.append(comp);
        }
    }
    
    // Remove connections involving these components first
    // This includes connections to the component itself or any of its subcomponents
    QList<Connection*> connectionsToRemove;
    for (Component* comp : componentsToRemove) {
        for (Connection* conn : m_connections) {
            bool shouldRemove = false;
            
            // Check if connection involves this component directly
            if (conn->getSource() == comp || conn->getTarget() == comp) {
                shouldRemove = true;
            }
            
            // Check if connection involves any subcomponents of this component
            if (!shouldRemove) {
                SubComponent* srcSub = conn->getSourceSub();
                SubComponent* tgtSub = conn->getTargetSub();
                
                if (srcSub && srcSub->parentComponent() == comp) {
                    shouldRemove = true;
                }
                if (tgtSub && tgtSub->parentComponent() == comp) {
                    shouldRemove = true;
                }
            }
            
            if (shouldRemove && !connectionsToRemove.contains(conn)) {
                connectionsToRemove.append(conn);
            }
        }
    }
    
    for (Connection* conn : connectionsToRemove) {
        m_scene->removeItem(conn);
        m_connections.removeAll(conn);
        delete conn;
    }
    
    // Remove the components
    for (Component* comp : componentsToRemove) {
        QString id = comp->getId();
        m_componentMap.remove(id);
        m_scene->removeItem(comp);
        emit componentRemoved(id, typeId);
        delete comp;
    }
    
    qDebug() << "[Canvas] Removed" << componentsToRemove.size() 
             << "components and" << connectionsToRemove.size() << "connections";
}

void Canvas::notifyComponentEdited(const QString& id, const QString& typeId)
{
    qDebug() << "[Canvas] Component edited:" << id << "type:" << typeId;
    emit componentEdited(id, typeId);
}

void Canvas::setMode(CanvasMode mode)
{
    m_mode = mode;
    
    if (m_mode == CanvasMode::Select) {
        setCursor(Qt::ArrowCursor);
        // Cancel any pending connection
        if (m_pendingLine) {
            m_scene->removeItem(m_pendingLine);
            delete m_pendingLine;
            m_pendingLine = nullptr;
        }
        m_connectionSourceItem = nullptr;
        m_isDrawingConnection = false;
    } else if (m_mode == CanvasMode::Connect) {
        setCursor(Qt::CrossCursor);
    }
    
    emit modeChanged(mode);
}

Connection* Canvas::addConnection(Component* source, Component* target,
                                   ConnectionType type, const QString& label)
{
    if (!source || !target || source == target) return nullptr;
    
    // Check if connection already exists
    for (Connection* conn : m_connections) {
        if ((conn->getSource() == source && conn->getTarget() == target) ||
            (conn->getSource() == target && conn->getTarget() == source)) {
            qDebug() << "[Canvas] Connection already exists between these components";
            return nullptr;
        }
    }
    
    Connection* conn = new Connection(source, target, type, label);
    conn->setId(QString("connection_%1").arg(++m_connectionCounter));
    
    m_scene->addItem(conn);
    m_connections.append(conn);
    
    qDebug() << "[Canvas] Connection created:" << conn->getId() 
             << "from" << source->getId() << "to" << target->getId()
             << "type:" << Connection::connectionTypeToString(type)
             << "label:" << label;
    
    emit connectionAdded(conn);
    return conn;
}

Connection* Canvas::addConnectionBetweenItems(QGraphicsItem* source, QGraphicsItem* target,
                                              ConnectionType type, const QString& label)
{
    if (!source || !target || source == target) return nullptr;
    
    // Check if connection already exists
    for (Connection* conn : m_connections) {
        if ((conn->getSourceItem() == source && conn->getTargetItem() == target) ||
            (conn->getSourceItem() == target && conn->getTargetItem() == source)) {
            qDebug() << "[Canvas] Connection already exists between these items";
            return nullptr;
        }
    }
    
    Connection* conn = new Connection(source, target, type, label);
    conn->setId(QString("connection_%1").arg(++m_connectionCounter));
    
    m_scene->addItem(conn);
    m_connections.append(conn);
    
    // Log the connection with appropriate details
    QString sourceDesc = "item";
    QString targetDesc = "item";
    
    Component* srcComp = qgraphicsitem_cast<Component*>(source);
    Component* tgtComp = qgraphicsitem_cast<Component*>(target);
    SubComponent* srcSub = qgraphicsitem_cast<SubComponent*>(source);
    SubComponent* tgtSub = qgraphicsitem_cast<SubComponent*>(target);
    
    if (srcComp) sourceDesc = QString("Component(%1)").arg(srcComp->getId());
    else if (srcSub) sourceDesc = QString("SubComponent(%1)").arg(srcSub->getName());
    
    if (tgtComp) targetDesc = QString("Component(%1)").arg(tgtComp->getId());
    else if (tgtSub) targetDesc = QString("SubComponent(%1)").arg(tgtSub->getName());
    
    qDebug() << "[Canvas] Connection created:" << conn->getId() 
             << "from" << sourceDesc << "to" << targetDesc
             << "type:" << Connection::connectionTypeToString(type)
             << "label:" << label;
    
    emit connectionAdded(conn);
    return conn;
}

void Canvas::removeConnection(Connection* conn)
{
    if (!conn) return;
    
    m_connections.removeOne(conn);
    m_scene->removeItem(conn);
    delete conn;
}

void Canvas::removeConnectionsInvolvingItem(QGraphicsItem* item)
{
    if (!item) return;
    
    QList<Connection*> connectionsToRemove;
    for (Connection* conn : m_connections) {
        if (conn->involvesItem(item)) {
            connectionsToRemove.append(conn);
        }
    }
    
    for (Connection* conn : connectionsToRemove) {
        m_scene->removeItem(conn);
        m_connections.removeAll(conn);
        delete conn;
    }
    
    if (!connectionsToRemove.isEmpty()) {
        qDebug() << "[Canvas] Removed" << connectionsToRemove.size() 
                 << "connections involving item";
    }
}

void Canvas::removeConnectionsInvolvingSubComponent(SubComponent* sub)
{
    if (!sub) return;
    
    QList<Connection*> connectionsToRemove;
    for (Connection* conn : m_connections) {
        if (conn->involvesSubComponent(sub)) {
            connectionsToRemove.append(conn);
        }
    }
    
    for (Connection* conn : connectionsToRemove) {
        m_scene->removeItem(conn);
        m_connections.removeAll(conn);
        delete conn;
    }
    
    if (!connectionsToRemove.isEmpty()) {
        qDebug() << "[Canvas] Removed" << connectionsToRemove.size() 
                 << "connections involving SubComponent:" << sub->getName();
    }
}

void Canvas::deleteSelectedConnections()
{
    QList<Connection*> toRemove;
    for (Connection* conn : m_connections) {
        if (conn->isSelected()) {
            toRemove.append(conn);
        }
    }
    for (Connection* conn : toRemove) {
        removeConnection(conn);
    }
}

void Canvas::dragEnterEvent(QDragEnterEvent* event)
{
    if (m_readOnly) { event->ignore(); return; }
    
    if (event->mimeData()->hasText() || 
        event->mimeData()->hasFormat("application/x-component-typeid")) {
        event->acceptProposedAction();
    }
}

void Canvas::dragMoveEvent(QDragMoveEvent* event)
{
    if (m_readOnly) { event->ignore(); return; }
    
    if (event->mimeData()->hasText() || 
        event->mimeData()->hasFormat("application/x-component-typeid")) {
        event->acceptProposedAction();
    }
}

void Canvas::dropEvent(QDropEvent* event)
{
    if (m_readOnly) { event->ignore(); return; }
    
    QString mimeText = event->mimeData()->hasText() ? event->mimeData()->text() : QString();
    
    // ------------------------------------------------------------------
    // Sub-component (design widget) drop
    // ------------------------------------------------------------------
    if (mimeText.startsWith("subcomponent:")) {
        QString subTypeStr = mimeText.mid(13);  // after "subcomponent:"
        SubComponentType subType = DesignSubComponent::stringToType(subTypeStr);
        
        QPointF scenePos = mapToScene(event->pos());
        
        // Find the parent component at the drop position
        Component* parentComp = componentAtScenePos(scenePos);
        
        if (!parentComp) {
            emit dropRejected("Sub-components must be dropped inside a parent component.\n"
                              "Drag this item onto an existing component on the canvas.");
            return;
        }
        
        // Validate
        if (!parentComp->canAcceptDesignSubComponent(subType)) {
            QString msg = Component::widgetValidationMessage(parentComp->getTypeId(), subType);
            emit dropRejected(msg);
            return;
        }
        
        // Determine default text
        QString defaultText;
        switch (subType) {
        case SubComponentType::Label:    defaultText = "Label";        break;
        case SubComponentType::LineEdit: defaultText = "Enter text..."; break;
        case SubComponentType::Button:   defaultText = "Click Me";     break;
        }
        
        // Create the design sub-component
        DesignSubComponent* sub = new DesignSubComponent(subType, defaultText);
        
        // Add to parent first (sets parentItem)
        parentComp->addDesignSubComponent(sub);
        
        // Compute position in parent coordinates
        QRectF container = parentComp->designContainerRect();
        QPointF localPos = parentComp->mapFromScene(scenePos);
        
        // Clamp to container body (skip header)
        qreal headerOffset = 18.0;
        localPos.setX(qBound(container.left(),
                              localPos.x() - sub->getWidth() / 2.0,
                              container.right() - sub->getWidth()));
        localPos.setY(qBound(container.top() + headerOffset,
                              localPos.y() - sub->getHeight() / 2.0,
                              container.bottom() - sub->getHeight()));
        
        sub->setPos(localPos);
        
        emit designSubComponentAdded(parentComp->getId(), subType);
        event->acceptProposedAction();
        return;
    }
    
    // ------------------------------------------------------------------
    // Main component drop
    // ------------------------------------------------------------------
    ComponentRegistry& registry = ComponentRegistry::instance();
    QString typeId;
    
    // Try custom mime type first (preferred)
    if (event->mimeData()->hasFormat("application/x-component-typeid")) {
        typeId = QString::fromUtf8(event->mimeData()->data("application/x-component-typeid"));
    }
    
    // Fall back to text (display name) and resolve via registry
    if (typeId.isEmpty() && event->mimeData()->hasText()) {
        QString text = event->mimeData()->text();
        typeId = registry.resolveTypeId(text);
    }
    
    // Verify type is registered
    if (typeId.isEmpty() || !registry.hasComponent(typeId)) {
        qWarning() << "[Canvas] Drop rejected - unknown type from mime data";
        return;
    }
    
    QString id = QString("component_%1").arg(++m_componentCounter);
    Component* comp = new Component(typeId, id);
    
    QPointF scenePos = mapToScene(event->pos());
    comp->setPos(scenePos);
    
    m_scene->addItem(comp);
    m_componentMap[id] = comp;
    emit componentAdded(id, typeId);
    
    event->acceptProposedAction();
}

void Canvas::mousePressEvent(QMouseEvent* event)
{
    if (m_readOnly) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    
    if (m_mode == CanvasMode::Connect && event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsItem* item = connectableItemAtPoint(scenePos);
        
        if (item) {
            m_connectionSourceItem = item;
            m_isDrawingConnection = true;
            
            // Create a temporary line to show the pending connection
            ThemeManager& tm = ThemeManager::instance();
            QPointF startPos = item->sceneBoundingRect().center();
            QColor pendingColor = tm.connectionDefaultColor();
            pendingColor.setAlpha(150);
            m_pendingLine = m_scene->addLine(
                QLineF(startPos, startPos),
                QPen(pendingColor, 2, Qt::DashLine));
            m_pendingLine->setZValue(-2);
            
            return;  // Don't pass to base class
        }
    }
    
    QGraphicsView::mousePressEvent(event);
    
    // Update connections when items are being moved
    if (m_mode == CanvasMode::Select) {
        updateAllConnections();
    }
}

void Canvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDrawingConnection && m_pendingLine && m_connectionSourceItem) {
        QPointF scenePos = mapToScene(event->pos());
        QPointF startPos = m_connectionSourceItem->sceneBoundingRect().center();
        m_pendingLine->setLine(QLineF(startPos, scenePos));
        return;
    }
    
    QGraphicsView::mouseMoveEvent(event);
    
    // Update connections while dragging items
    updateAllConnections();
}

void Canvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isDrawingConnection && m_connectionSourceItem && event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsItem* target = connectableItemAtPoint(scenePos);
        
        // Remove the pending line
        if (m_pendingLine) {
            m_scene->removeItem(m_pendingLine);
            delete m_pendingLine;
            m_pendingLine = nullptr;
        }
        
        if (target && target != m_connectionSourceItem) {
            // Ask for connection label
            QString label = m_pendingConnectionLabel;
            if (label.isEmpty()) {
                bool ok;
                label = QInputDialog::getText(this, "Connection Label",
                    "Enter label for the connection (or leave empty):",
                    QLineEdit::Normal, "", &ok);
                if (!ok) {
                    m_connectionSourceItem = nullptr;
                    m_isDrawingConnection = false;
                    return;
                }
            }
            
            addConnectionBetweenItems(m_connectionSourceItem, target, m_pendingConnectionType, label);
        }
        
        m_connectionSourceItem = nullptr;
        m_isDrawingConnection = false;
        return;
    }
    
    QGraphicsView::mouseReleaseEvent(event);
    updateAllConnections();
}

void Canvas::keyPressEvent(QKeyEvent* event)
{
    if (m_readOnly) {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Delete selected design sub-components first
        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        foreach (QGraphicsItem* item, selected) {
            DesignSubComponent* dsub = dynamic_cast<DesignSubComponent*>(item);
            if (dsub) {
                Component* parent = dynamic_cast<Component*>(dsub->parentItem());
                if (parent) {
                    parent->removeDesignSubComponent(dsub);
                }
                delete dsub;
            }
        }
        // Also delete selected connections
        deleteSelectedConnections();
    } else if (event->key() == Qt::Key_Escape) {
        if (m_mode == CanvasMode::Connect) {
            setMode(CanvasMode::Select);
        }
        if (m_pendingLine) {
            m_scene->removeItem(m_pendingLine);
            delete m_pendingLine;
            m_pendingLine = nullptr;
        }
        m_connectionSourceItem = nullptr;
        m_isDrawingConnection = false;
    }
    
    QGraphicsView::keyPressEvent(event);
}

void Canvas::updateAllConnections()
{
    for (Connection* conn : m_connections) {
        conn->updatePosition();
    }
}

Component* Canvas::componentAtPoint(const QPointF& scenePos)
{
    QList<QGraphicsItem*> items = m_scene->items(scenePos);
    for (QGraphicsItem* item : items) {
        Component* comp = dynamic_cast<Component*>(item);
        if (comp) return comp;
        
        // Check if we clicked on a sub-component - return its parent
        SubComponent* sub = dynamic_cast<SubComponent*>(item);
        if (sub && sub->parentItem()) {
            Component* parentComp = dynamic_cast<Component*>(sub->parentItem());
            if (parentComp) return parentComp;
        }
        
        // Check if we clicked on a design sub-component - return its parent
        DesignSubComponent* dsub = dynamic_cast<DesignSubComponent*>(item);
        if (dsub && dsub->parentItem()) {
            Component* parentComp = dynamic_cast<Component*>(dsub->parentItem());
            if (parentComp) return parentComp;
        }
    }
    return nullptr;
}

// ======================================================================
// Helper – find connectable item (Component or SubComponent) at scene position
// ======================================================================

QGraphicsItem* Canvas::connectableItemAtPoint(const QPointF& scenePos)
{
    QList<QGraphicsItem*> items = m_scene->items(scenePos);
    
    // Prioritize SubComponents (they are drawn on top of Components)
    for (QGraphicsItem* item : items) {
        SubComponent* sub = qgraphicsitem_cast<SubComponent*>(item);
        if (sub) {
            return sub;
        }
    }
    
    // If no SubComponent found, look for a Component
    for (QGraphicsItem* item : items) {
        Component* comp = qgraphicsitem_cast<Component*>(item);
        if (comp) {
            return comp;
        }
    }
    
    return nullptr;
}

// ======================================================================
// Helper – find Component at scene position for sub-component drops
// ======================================================================

Component* Canvas::componentAtScenePos(const QPointF& scenePos) const
{
    QList<QGraphicsItem*> itemsAtPos = m_scene->items(scenePos);
    
    foreach (QGraphicsItem* item, itemsAtPos) {
        // Direct Component hit
        Component* comp = dynamic_cast<Component*>(item);
        if (comp) return comp;
        
        // If hit a SubComponent, return its parent Component
        SubComponent* sub = dynamic_cast<SubComponent*>(item);
        if (sub) {
            Component* parent = dynamic_cast<Component*>(sub->parentItem());
            if (parent) return parent;
        }
        
        // If hit a DesignSubComponent, return its parent Component
        DesignSubComponent* dsub = dynamic_cast<DesignSubComponent*>(item);
        if (dsub) {
            Component* parent = dynamic_cast<Component*>(dsub->parentItem());
            if (parent) return parent;
        }
    }
    
    // Also check if the scenePos is inside any Component's bounding rect
    // (needed for the design container area which might not have items yet)
    foreach (QGraphicsItem* item, m_scene->items()) {
        Component* comp = dynamic_cast<Component*>(item);
        if (comp) {
            QPointF local = comp->mapFromScene(scenePos);
            if (comp->boundingRect().contains(local)) {
                return comp;
            }
            // Also check the design container rect (for dropping into empty area)
            if (comp->designContainerRect().contains(local)) {
                return comp;
            }
        }
    }
    
    return nullptr;
}

QString Canvas::saveToJson() const
{
    QJsonArray componentsArray;
    
    foreach (Component* comp, getComponents()) {
        QJsonObject compObj;
        compObj["id"] = comp->getId();
        compObj["type"] = comp->getTypeId();
        
        // Only save displayName and label if they've been customized
        if (comp->hasCustomDisplayName()) {
            compObj["displayName"] = comp->getDisplayName();
        }
        if (comp->hasCustomLabel()) {
            compObj["label"] = comp->getLabel();
        }
        
        compObj["x"] = comp->pos().x();
        compObj["y"] = comp->pos().y();
        compObj["color"] = comp->getColor().name();
        compObj["size"] = comp->getSize();
        compObj["userWidth"] = comp->getUserWidth();
        compObj["userHeight"] = comp->getUserHeight();
        compObj["minimized"] = comp->isMinimized();
        
        // Save sub-components (health-tracking subsystems)
        QJsonArray subArray;
        for (SubComponent* sub : comp->getSubComponents()) {
            QJsonObject subObj;
            subObj["name"] = sub->getName();
            subObj["health"] = sub->getHealth();
            subObj["color"] = sub->getColor().name();
            subObj["x"] = sub->pos().x();
            subObj["y"] = sub->pos().y();
            subObj["width"] = sub->getWidth();
            subObj["height"] = sub->getHeight();
            subArray.append(subObj);
        }
        compObj["subcomponents"] = subArray;
        
        // Save design sub-components (drag-drop widgets)
        QJsonArray designSubArray;
        for (DesignSubComponent* dsub : comp->getDesignSubComponents()) {
            QJsonObject dsubObj;
            dsubObj["type"]   = DesignSubComponent::typeToString(dsub->getType());
            dsubObj["text"]   = dsub->getText();
            dsubObj["x"]      = dsub->pos().x();
            dsubObj["y"]      = dsub->pos().y();
            dsubObj["width"]  = dsub->getWidth();
            dsubObj["height"] = dsub->getHeight();
            dsubObj["healthColor"] = dsub->getHealthColor().name();
            dsubObj["healthValue"] = dsub->getHealthValue();
            designSubArray.append(dsubObj);
        }
        if (!designSubArray.isEmpty()) {
            compObj["design_subcomponents"] = designSubArray;
        }
        
        componentsArray.append(compObj);
    }
    
    // Save connections
    QJsonArray connectionsArray;
    for (Connection* conn : m_connections) {
        QJsonObject connObj;
        connObj["id"] = conn->getId();
        
        // Handle source - could be Component or SubComponent
        Component* srcComp = conn->getSource();
        SubComponent* srcSub = conn->getSourceSub();
        if (srcComp) {
            connObj["source"] = srcComp->getId();
            connObj["sourceType"] = "component";
        } else if (srcSub) {
            Component* parentComp = srcSub->parentComponent();
            if (parentComp) {
                connObj["source"] = parentComp->getId();
                connObj["sourceSubComponent"] = srcSub->getName();
                connObj["sourceType"] = "subcomponent";
            } else {
                qWarning() << "[Canvas] SubComponent connection has no parent component (source)";
                continue; // Skip this connection
            }
        } else {
            qWarning() << "[Canvas] Connection has invalid source";
            continue;
        }
        
        // Handle target - could be Component or SubComponent
        Component* tgtComp = conn->getTarget();
        SubComponent* tgtSub = conn->getTargetSub();
        if (tgtComp) {
            connObj["target"] = tgtComp->getId();
            connObj["targetType"] = "component";
        } else if (tgtSub) {
            Component* parentComp = tgtSub->parentComponent();
            if (parentComp) {
                connObj["target"] = parentComp->getId();
                connObj["targetSubComponent"] = tgtSub->getName();
                connObj["targetType"] = "subcomponent";
            } else {
                qWarning() << "[Canvas] SubComponent connection has no parent component (target)";
                continue; // Skip this connection
            }
        } else {
            qWarning() << "[Canvas] Connection has invalid target";
            continue;
        }
        
        connObj["type"] = Connection::connectionTypeToString(conn->getConnectionType());
        connObj["label"] = conn->getLabel();
        connObj["color"] = conn->getColor().name();
        connectionsArray.append(connObj);
    }
    
    QJsonObject root;
    root["components"] = componentsArray;
    root["connections"] = connectionsArray;
    
    QJsonDocument doc(root);
    return doc.toJson();
}

void Canvas::loadFromJson(const QString& json)
{
    qDebug() << "[Canvas] Starting loadFromJson";
    clearCanvas();
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "[Canvas] Failed to parse JSON or not an object";
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray componentsArray = root["components"].toArray();
    qDebug() << "[Canvas] Found" << componentsArray.size() << "components to load";
    
    ComponentRegistry& registry = ComponentRegistry::instance();
    
    // Load components
    foreach (const QJsonValue& value, componentsArray) {
        QJsonObject compObj = value.toObject();
        
        QString id = compObj["id"].toString();
        QString typeStr = compObj["type"].toString();
        qreal x = compObj["x"].toDouble();
        qreal y = compObj["y"].toDouble();
        QString colorStr = compObj["color"].toString();
        qreal size = compObj["size"].toDouble();
        
        // Resolve typeId - handles both new format and legacy format
        QString typeId = typeStr;
        if (!registry.hasComponent(typeId)) {
            typeId = registry.resolveTypeId(typeStr);
        }
        
        if (typeId.isEmpty() || !registry.hasComponent(typeId)) {
            qWarning() << "[Canvas] Unknown component type in design file:" << typeStr 
                       << "- creating with raw type ID";
            typeId = typeStr;
        }
        
        Component* comp = Component::fromJson(id, typeId, x, y, QColor(colorStr), size);
        
        // Restore per-component display name and label if present
        if (compObj.contains("displayName")) {
            QString displayName = compObj["displayName"].toString();
            if (!displayName.isEmpty()) {
                comp->setDisplayName(displayName);
            }
        }
        if (compObj.contains("label")) {
            QString label = compObj["label"].toString();
            if (!label.isEmpty()) {
                comp->setLabel(label);
            }
        }
        
        // Restore user-defined dimensions if present
        if (compObj.contains("userWidth") && compObj.contains("userHeight")) {
            qreal uw = compObj["userWidth"].toDouble();
            qreal uh = compObj["userHeight"].toDouble();
            if (uw > 0) comp->setUserWidth(uw);
            if (uh > 0) comp->setUserHeight(uh);
        }
        
        // Restore minimized state if present
        if (compObj.contains("minimized")) {
            bool minimized = compObj["minimized"].toBool();
            comp->setMinimized(minimized);
        }
        
        m_scene->addItem(comp);
        m_componentMap[id] = comp;
        
        // Load sub-component states if present
        QJsonArray subArray = compObj["subcomponents"].toArray();
        for (int i = 0; i < subArray.size() && i < comp->subComponentCount(); ++i) {
            QJsonObject subObj = subArray[i].toObject();
            SubComponent* sub = comp->getSubComponents()[i];
            if (sub) {
                sub->setHealth(subObj["health"].toDouble(100));
                sub->setColor(QColor(subObj["color"].toString("#4CAF50")));
                
                // Restore position and size if present
                if (subObj.contains("x") && subObj.contains("y")) {
                    sub->setPos(subObj["x"].toDouble(), subObj["y"].toDouble());
                }
                if (subObj.contains("width")) {
                    sub->setWidth(subObj["width"].toDouble());
                }
                if (subObj.contains("height")) {
                    sub->setHeight(subObj["height"].toDouble());
                }
            }
        }
        
        // Load design sub-components (drag-drop widgets)
        QJsonArray designSubArray = compObj["design_subcomponents"].toArray();
        foreach (const QJsonValue& dsubVal, designSubArray) {
            QJsonObject dsubObj = dsubVal.toObject();
            SubComponentType subType = DesignSubComponent::stringToType(dsubObj["type"].toString());
            QString text = dsubObj["text"].toString();
            qreal sx     = dsubObj["x"].toDouble();
            qreal sy     = dsubObj["y"].toDouble();
            qreal sw     = dsubObj["width"].toDouble();
            qreal sh     = dsubObj["height"].toDouble();
            
            DesignSubComponent* dsub = new DesignSubComponent(subType, text);
            dsub->setSize(sw, sh);
            
            // Restore health properties
            if (dsubObj.contains("healthColor")) {
                dsub->setHealthColor(QColor(dsubObj["healthColor"].toString("#4CAF50")));
            }
            if (dsubObj.contains("healthValue")) {
                dsub->setHealthValue(dsubObj["healthValue"].toDouble(100.0));
            }
            
            comp->addDesignSubComponent(dsub);   // sets parent
            dsub->setPos(sx, sy);                 // in parent coords
        }
        
        qDebug() << "[Canvas] Loaded component" << id << "of type" << typeId 
                 << "at (" << x << "," << y << ")"
                 << "with" << comp->subComponentCount() << "sub-components"
                 << "and" << comp->designSubComponentCount() << "design sub-components";
        
        emit componentLoaded(id, typeId);
        
        // Update counter
        if (id.startsWith("component_")) {
            int num = id.mid(10).toInt();
            if (num >= m_componentCounter) {
                m_componentCounter = num;
            }
        }
    }
    
    // Load connections
    QJsonArray connectionsArray = root["connections"].toArray();
    for (const QJsonValue& value : connectionsArray) {
        QJsonObject connObj = value.toObject();
        
        QString sourceId = connObj["source"].toString();
        QString targetId = connObj["target"].toString();
        QString sourceType = connObj["sourceType"].toString("component"); // Default to component for backwards compatibility
        QString targetType = connObj["targetType"].toString("component");
        QString typeStr = connObj["type"].toString("unidirectional");
        QString label = connObj["label"].toString();
        QString colorStr = connObj["color"].toString();
        
        QGraphicsItem* sourceItem = nullptr;
        QGraphicsItem* targetItem = nullptr;
        
        // Resolve source item
        if (sourceType == "subcomponent") {
            QString subComponentName = connObj["sourceSubComponent"].toString();
            Component* parentComp = getComponentById(sourceId);
            if (parentComp) {
                sourceItem = parentComp->getSubComponent(subComponentName);
                if (!sourceItem) {
                    qWarning() << "[Canvas] Could not find source SubComponent:" << subComponentName
                               << "in component:" << sourceId;
                }
            } else {
                qWarning() << "[Canvas] Could not find parent component for source SubComponent:" << sourceId;
            }
        } else {
            sourceItem = getComponentById(sourceId);
            if (!sourceItem) {
                qWarning() << "[Canvas] Could not find source Component:" << sourceId;
            }
        }
        
        // Resolve target item
        if (targetType == "subcomponent") {
            QString subComponentName = connObj["targetSubComponent"].toString();
            Component* parentComp = getComponentById(targetId);
            if (parentComp) {
                targetItem = parentComp->getSubComponent(subComponentName);
                if (!targetItem) {
                    qWarning() << "[Canvas] Could not find target SubComponent:" << subComponentName
                               << "in component:" << targetId;
                }
            } else {
                qWarning() << "[Canvas] Could not find parent component for target SubComponent:" << targetId;
            }
        } else {
            targetItem = getComponentById(targetId);
            if (!targetItem) {
                qWarning() << "[Canvas] Could not find target Component:" << targetId;
            }
        }
        
        // Create the connection if both endpoints were found
        if (sourceItem && targetItem) {
            ConnectionType type = Connection::stringToConnectionType(typeStr);
            Connection* conn = addConnectionBetweenItems(sourceItem, targetItem, type, label);
            if (conn && !colorStr.isEmpty()) {
                conn->setColor(QColor(colorStr));
            }
        } else {
            qWarning() << "[Canvas] Could not load connection: source=" << sourceId 
                       << "(" << sourceType << ")"
                       << "target=" << targetId
                       << "(" << targetType << ")";
        }
    }
    
    // Re-apply read-only state to newly loaded components
    if (m_readOnly) {
        for (auto it = m_componentMap.begin(); it != m_componentMap.end(); ++it) {
            Component* comp = it.value();
            if (comp) {
                comp->setFlag(QGraphicsItem::ItemIsMovable, false);
                comp->setFlag(QGraphicsItem::ItemIsSelectable, false);
            }
        }
    }
    
    qDebug() << "[Canvas] loadFromJson complete. Components:" << m_componentMap.size() 
             << "Connections:" << m_connections.size();
}

// ======================================================================
// Import/Export single components
// ======================================================================

bool Canvas::importComponent(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[Canvas] Could not open component file:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[Canvas] Invalid component file format";
        return false;
    }
    
    QJsonObject json = doc.object();
    
    // Verify it's a component file
    if (json["type"].toString() != "component") {
        qWarning() << "[Canvas] Not a valid component file";
        return false;
    }
    
    // Extract component data
    QString typeId = json["typeId"].toString();
    QColor color = QColor(json["color"].toString());
    qreal size = json["size"].toDouble(100.0);
    qreal userWidth = json["userWidth"].toDouble(0);
    qreal userHeight = json["userHeight"].toDouble(0);
    
    // Check if component type exists in registry
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (!registry.hasComponent(typeId)) {
        // Try to import component definition if it's included in the file
        if (json.contains("componentDefinition")) {
            QJsonObject compDefJson = json["componentDefinition"].toObject();
            ComponentDefinition compDef = ComponentDefinition::fromJson(compDefJson);
            
            if (compDef.typeId == typeId) {
                qDebug() << "[Canvas] Registering new component type from import:" << typeId;
                if (registry.registerComponent(compDef)) {
                    // Save registry to persist the new component type
                    if (registry.saveToFile()) {
                        qDebug() << "[Canvas] Component type registered and saved:" << typeId;
                    } else {
                        qWarning() << "[Canvas] Failed to save registry after registering component type";
                    }
                } else {
                    qWarning() << "[Canvas] Failed to register component type:" << typeId;
                    return false;
                }
            } else {
                qWarning() << "[Canvas] Component definition typeId mismatch:" << compDef.typeId << "vs" << typeId;
                return false;
            }
        } else {
            qWarning() << "[Canvas] Unknown component type and no definition provided:" << typeId;
            return false;
        }
    }
    
    // Create new component with unique ID
    QString id = QString("component_%1").arg(++m_componentCounter);
    Component* comp = new Component(typeId, id);
    
    // Set properties
    comp->setColor(color);
    comp->setSize(size);
    if (userWidth > 0) comp->setUserWidth(userWidth);
    if (userHeight > 0) comp->setUserHeight(userHeight);
    
    // Position at center of view
    QPointF centerPos = mapToScene(viewport()->rect().center());
    comp->setPos(centerPos);
    
    // Clear default subsystems and add imported ones
    // Important: Clean up connections before removing subcomponents to avoid crashes
    while (comp->subComponentCount() > 0) {
        SubComponent* sub = comp->getSubComponents()[0];
        if (sub) {
            removeConnectionsInvolvingSubComponent(sub);
        }
        comp->removeSubComponent(0);
    }
    
    QJsonArray subsystems = json["subsystems"].toArray();
    for (const QJsonValue& subVal : subsystems) {
        QJsonObject subObj = subVal.toObject();
        QString name = subObj["name"].toString();
        comp->addSubComponent(name);
        
        // Set health and color
        SubComponent* sub = comp->getSubComponent(name);
        if (sub) {
            sub->setHealth(subObj["health"].toDouble(100.0));
            sub->setColor(QColor(subObj["color"].toString()));
        }
    }
    
    // Import design widgets
    QJsonArray widgets = json["designWidgets"].toArray();
    for (const QJsonValue& widgetVal : widgets) {
        QJsonObject widgetObj = widgetVal.toObject();
        SubComponentType subType = DesignSubComponent::stringToType(widgetObj["type"].toString());
        QString text = widgetObj["text"].toString();
        qreal x = widgetObj["x"].toDouble();
        qreal y = widgetObj["y"].toDouble();
        qreal width = widgetObj["width"].toDouble();
        qreal height = widgetObj["height"].toDouble();
        
        DesignSubComponent* widget = new DesignSubComponent(subType, text);
        widget->setSize(width, height);
        comp->addDesignSubComponent(widget);
        widget->setPos(x, y);
    }
    
    // Add to scene
    m_scene->addItem(comp);
    m_componentMap[id] = comp;
    
    qDebug() << "[Canvas] Imported component" << id << "from" << filePath;
    emit componentAdded(id, typeId);
    
    return true;
}

bool Canvas::importSubcomponents(const QString& filePath, Component* targetComponent)
{
    if (!targetComponent) {
        qWarning() << "[Canvas] No target component specified";
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[Canvas] Could not open subcomponent file:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[Canvas] Invalid subcomponent file format";
        return false;
    }
    
    QJsonObject json = doc.object();
    
    // Verify it's a subcomponents file
    if (json["type"].toString() != "subcomponents") {
        qWarning() << "[Canvas] Not a valid subcomponents file";
        return false;
    }
    
    // Import design widgets
    QJsonArray widgets = json["widgets"].toArray();
    int importedCount = 0;
    
    for (const QJsonValue& widgetVal : widgets) {
        QJsonObject widgetObj = widgetVal.toObject();
        SubComponentType subType = DesignSubComponent::stringToType(widgetObj["type"].toString());
        QString text = widgetObj["text"].toString();
        qreal x = widgetObj["x"].toDouble();
        qreal y = widgetObj["y"].toDouble();
        qreal width = widgetObj["width"].toDouble();
        qreal height = widgetObj["height"].toDouble();
        
        // Check if this widget type is allowed in the target component
        if (!targetComponent->canAcceptDesignSubComponent(subType)) {
            qWarning() << "[Canvas] Widget type" << DesignSubComponent::typeToString(subType)
                      << "not allowed in component" << targetComponent->getId();
            continue;
        }
        
        DesignSubComponent* widget = new DesignSubComponent(subType, text);
        widget->setSize(width, height);
        targetComponent->addDesignSubComponent(widget);
        widget->setPos(x, y);
        importedCount++;
    }
    
    qDebug() << "[Canvas] Imported" << importedCount << "widgets from" << filePath 
             << "into component" << targetComponent->getId();
    
    return importedCount > 0;
}
