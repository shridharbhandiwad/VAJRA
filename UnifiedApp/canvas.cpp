#include "canvas.h"
#include "componentregistry.h"
#include "thememanager.h"
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QInputDialog>

Canvas::Canvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_componentCounter(0)
    , m_connectionCounter(0)
    , m_mode(CanvasMode::Select)
    , m_pendingConnectionType(ConnectionType::Unidirectional)
    , m_connectionSource(nullptr)
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
}

Canvas::~Canvas()
{
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
    m_connectionSource = nullptr;
    m_pendingLine = nullptr;
    m_isDrawingConnection = false;
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
        m_connectionSource = nullptr;
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

void Canvas::removeConnection(Connection* conn)
{
    if (!conn) return;
    
    m_connections.removeOne(conn);
    m_scene->removeItem(conn);
    delete conn;
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
    if (event->mimeData()->hasText() || 
        event->mimeData()->hasFormat("application/x-component-typeid")) {
        event->acceptProposedAction();
    }
}

void Canvas::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasText() || 
        event->mimeData()->hasFormat("application/x-component-typeid")) {
        event->acceptProposedAction();
    }
}

void Canvas::dropEvent(QDropEvent* event)
{
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
    if (m_mode == CanvasMode::Connect && event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        Component* comp = componentAtPoint(scenePos);
        
        if (comp) {
            m_connectionSource = comp;
            m_isDrawingConnection = true;
            
            // Create a temporary line to show the pending connection
            QPointF startPos = comp->sceneBoundingRect().center();
            m_pendingLine = m_scene->addLine(
                QLineF(startPos, startPos),
                QPen(QColor(100, 180, 220, 150), 2, Qt::DashLine));
            m_pendingLine->setZValue(-2);
            
            return;  // Don't pass to base class
        }
    }
    
    QGraphicsView::mousePressEvent(event);
    
    // Update connections when components are being moved
    if (m_mode == CanvasMode::Select) {
        updateAllConnections();
    }
}

void Canvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDrawingConnection && m_pendingLine && m_connectionSource) {
        QPointF scenePos = mapToScene(event->pos());
        QPointF startPos = m_connectionSource->sceneBoundingRect().center();
        m_pendingLine->setLine(QLineF(startPos, scenePos));
        return;
    }
    
    QGraphicsView::mouseMoveEvent(event);
    
    // Update connections while dragging components
    updateAllConnections();
}

void Canvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isDrawingConnection && m_connectionSource && event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        Component* target = componentAtPoint(scenePos);
        
        // Remove the pending line
        if (m_pendingLine) {
            m_scene->removeItem(m_pendingLine);
            delete m_pendingLine;
            m_pendingLine = nullptr;
        }
        
        if (target && target != m_connectionSource) {
            // Ask for connection label
            QString label = m_pendingConnectionLabel;
            if (label.isEmpty()) {
                bool ok;
                label = QInputDialog::getText(this, "Connection Label",
                    "Enter label for the connection (or leave empty):",
                    QLineEdit::Normal, "", &ok);
                if (!ok) {
                    m_connectionSource = nullptr;
                    m_isDrawingConnection = false;
                    return;
                }
            }
            
            addConnection(m_connectionSource, target, m_pendingConnectionType, label);
        }
        
        m_connectionSource = nullptr;
        m_isDrawingConnection = false;
        return;
    }
    
    QGraphicsView::mouseReleaseEvent(event);
    updateAllConnections();
}

void Canvas::keyPressEvent(QKeyEvent* event)
{
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
        m_connectionSource = nullptr;
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
        compObj["x"] = comp->pos().x();
        compObj["y"] = comp->pos().y();
        compObj["color"] = comp->getColor().name();
        compObj["size"] = comp->getSize();
        
        // Save sub-components (health-tracking subsystems)
        QJsonArray subArray;
        for (SubComponent* sub : comp->getSubComponents()) {
            QJsonObject subObj;
            subObj["name"] = sub->getName();
            subObj["health"] = sub->getHealth();
            subObj["color"] = sub->getColor().name();
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
        connObj["source"] = conn->getSource()->getId();
        connObj["target"] = conn->getTarget()->getId();
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
        QString typeStr = connObj["type"].toString("unidirectional");
        QString label = connObj["label"].toString();
        QString colorStr = connObj["color"].toString();
        
        Component* source = getComponentById(sourceId);
        Component* target = getComponentById(targetId);
        
        if (source && target) {
            ConnectionType type = Connection::stringToConnectionType(typeStr);
            Connection* conn = addConnection(source, target, type, label);
            if (conn && !colorStr.isEmpty()) {
                conn->setColor(QColor(colorStr));
            }
        } else {
            qWarning() << "[Canvas] Could not load connection: source=" << sourceId 
                       << "target=" << targetId;
        }
    }
    
    qDebug() << "[Canvas] loadFromJson complete. Components:" << m_componentMap.size() 
             << "Connections:" << m_connections.size();
}
