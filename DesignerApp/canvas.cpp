#include "canvas.h"
#include <QDragEnterEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>

Canvas::Canvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_componentCounter(0)
{
    setScene(m_scene);
    setAcceptDrops(true);
    setRenderHint(QPainter::Antialiasing);
    
    // Set scene size
    m_scene->setSceneRect(0, 0, 600, 500);
    
    // Set background
    setBackgroundBrush(QBrush(QColor(240, 240, 240)));
}

Canvas::~Canvas()
{
}

void Canvas::addComponent(ComponentType type)
{
    QString id = QString("component_%1").arg(++m_componentCounter);
    Component* comp = new Component(type, id);
    
    // Place at center of view
    QPointF centerPos = mapToScene(viewport()->rect().center());
    comp->setPos(centerPos);
    
    m_scene->addItem(comp);
    emit componentAdded(id, type);
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
    m_scene->clear();
    m_componentCounter = 0;
}

// ======================================================================
// Drag-and-drop
// ======================================================================

void Canvas::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void Canvas::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void Canvas::dropEvent(QDropEvent* event)
{
    QString mimeText = event->mimeData()->text();
    
    // ------------------------------------------------------------------
    // Sub-component drop
    // ------------------------------------------------------------------
    if (mimeText.startsWith("subcomponent:")) {
        QString subTypeStr = mimeText.mid(13);  // after "subcomponent:"
        SubComponentType subType = SubComponent::stringToType(subTypeStr);
        
        QPointF scenePos = mapToScene(event->pos());
        
        // Find the parent component at the drop position
        Component* parentComp = componentAtScenePos(scenePos);
        
        if (!parentComp) {
            emit dropRejected("Sub-components must be dropped inside a parent component.\n"
                              "Drag this item onto an existing radar subsystem on the canvas.");
            return;
        }
        
        // Validate
        if (!parentComp->canAcceptSubComponent(subType)) {
            QString msg = Component::validationMessage(parentComp->getType(), subType);
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
        
        // Create the sub-component
        SubComponent* sub = new SubComponent(subType, defaultText);
        
        // Add to parent first (sets parentItem)
        parentComp->addSubComponent(sub);
        
        // Compute position in parent coordinates
        QRectF container = parentComp->containerRect();
        QPointF localPos = parentComp->mapFromScene(scenePos);
        
        // Clamp to container body (skip 20px header)
        qreal headerOffset = 20.0;
        localPos.setX(qBound(container.left(),
                              localPos.x() - sub->getWidth() / 2.0,
                              container.right() - sub->getWidth()));
        localPos.setY(qBound(container.top() + headerOffset,
                              localPos.y() - sub->getHeight() / 2.0,
                              container.bottom() - sub->getHeight()));
        
        sub->setPos(localPos);
        
        emit subComponentAdded(parentComp->getId(), subType);
        event->acceptProposedAction();
        return;
    }
    
    // ------------------------------------------------------------------
    // Main component drop (original behaviour)
    // ------------------------------------------------------------------
    ComponentType type;
    
    if (mimeText == "Antenna") {
        type = ComponentType::Antenna;
    } else if (mimeText == "Power System") {
        type = ComponentType::PowerSystem;
    } else if (mimeText == "Liquid Cooling Unit") {
        type = ComponentType::LiquidCoolingUnit;
    } else if (mimeText == "Communication System") {
        type = ComponentType::CommunicationSystem;
    } else if (mimeText == "Radar Computer") {
        type = ComponentType::RadarComputer;
    } else {
        return;
    }
    
    QString id = QString("component_%1").arg(++m_componentCounter);
    Component* comp = new Component(type, id);
    
    QPointF scenePos = mapToScene(event->pos());
    comp->setPos(scenePos);
    
    m_scene->addItem(comp);
    emit componentAdded(id, type);
    
    event->acceptProposedAction();
}

// ======================================================================
// Keyboard
// ======================================================================

void Canvas::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        // Delete selected sub-components
        QList<QGraphicsItem*> selected = m_scene->selectedItems();
        foreach (QGraphicsItem* item, selected) {
            SubComponent* sub = dynamic_cast<SubComponent*>(item);
            if (sub) {
                Component* parent = dynamic_cast<Component*>(sub->parentItem());
                if (parent) {
                    parent->removeSubComponent(sub);
                }
                delete sub;
            }
        }
    }
    QGraphicsView::keyPressEvent(event);
}

// ======================================================================
// Helper – find Component at scene position (skips SubComponents)
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
    }
    
    // Also check if the scenePos is inside any Component's expanded bounding rect
    // (needed for the container area which might not have items yet)
    foreach (QGraphicsItem* item, m_scene->items()) {
        Component* comp = dynamic_cast<Component*>(item);
        if (comp) {
            QPointF local = comp->mapFromScene(scenePos);
            if (comp->boundingRect().contains(local)) {
                return comp;
            }
            // Also check the container rect even when no subcomponents exist yet
            // so the user can drop on a component's "potential" container zone
            if (comp->containerRect().contains(local)) {
                return comp;
            }
        }
    }
    
    return nullptr;
}

// ======================================================================
// Save / Load  (extended to include sub-components)
// ======================================================================

QString Canvas::saveToJson() const
{
    QJsonArray componentsArray;
    
    foreach (Component* comp, getComponents()) {
        QJsonObject compObj;
        compObj["id"] = comp->getId();
        
        QString typeStr;
        switch (comp->getType()) {
            case ComponentType::Antenna:             typeStr = "Antenna"; break;
            case ComponentType::PowerSystem:          typeStr = "PowerSystem"; break;
            case ComponentType::LiquidCoolingUnit:    typeStr = "LiquidCoolingUnit"; break;
            case ComponentType::CommunicationSystem:  typeStr = "CommunicationSystem"; break;
            case ComponentType::RadarComputer:        typeStr = "RadarComputer"; break;
        }
        compObj["type"] = typeStr;
        compObj["x"] = comp->pos().x();
        compObj["y"] = comp->pos().y();
        compObj["color"] = comp->getColor().name();
        compObj["size"] = comp->getSize();
        
        // Serialize child sub-components
        QJsonArray subArray;
        foreach (SubComponent* sub, comp->getSubComponents()) {
            QJsonObject subObj;
            subObj["type"]   = SubComponent::typeToString(sub->getType());
            subObj["text"]   = sub->getText();
            subObj["x"]      = sub->pos().x();
            subObj["y"]      = sub->pos().y();
            subObj["width"]  = sub->getWidth();
            subObj["height"] = sub->getHeight();
            subObj["healthColor"] = sub->getHealthColor().name();
            subObj["healthValue"] = sub->getHealthValue();
            subArray.append(subObj);
        }
        if (!subArray.isEmpty()) {
            compObj["subcomponents"] = subArray;
        }
        
        componentsArray.append(compObj);
    }
    
    QJsonObject root;
    root["components"] = componentsArray;
    
    QJsonDocument doc(root);
    return doc.toJson();
}

void Canvas::loadFromJson(const QString& json)
{
    clearCanvas();
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray componentsArray = root["components"].toArray();
    
    foreach (const QJsonValue& value, componentsArray) {
        QJsonObject compObj = value.toObject();
        
        QString id      = compObj["id"].toString();
        QString typeStr = compObj["type"].toString();
        qreal x         = compObj["x"].toDouble();
        qreal y         = compObj["y"].toDouble();
        QString colorStr= compObj["color"].toString();
        qreal size      = compObj["size"].toDouble();
        
        ComponentType type;
        if      (typeStr == "Antenna")             type = ComponentType::Antenna;
        else if (typeStr == "PowerSystem")          type = ComponentType::PowerSystem;
        else if (typeStr == "LiquidCoolingUnit")    type = ComponentType::LiquidCoolingUnit;
        else if (typeStr == "CommunicationSystem")  type = ComponentType::CommunicationSystem;
        else if (typeStr == "RadarComputer")        type = ComponentType::RadarComputer;
        else continue;
        
        Component* comp = Component::fromJson(id, type, x, y, QColor(colorStr), size);
        m_scene->addItem(comp);
        
        // Load child sub-components
        QJsonArray subArray = compObj["subcomponents"].toArray();
        foreach (const QJsonValue& subVal, subArray) {
            QJsonObject subObj = subVal.toObject();
            SubComponentType subType = SubComponent::stringToType(subObj["type"].toString());
            QString text = subObj["text"].toString();
            qreal sx     = subObj["x"].toDouble();
            qreal sy     = subObj["y"].toDouble();
            qreal sw     = subObj["width"].toDouble();
            qreal sh     = subObj["height"].toDouble();
            
            SubComponent* sub = new SubComponent(subType, text);
            sub->setSize(sw, sh);
            
            // Restore health properties
            if (subObj.contains("healthColor")) {
                sub->setHealthColor(QColor(subObj["healthColor"].toString("#4CAF50")));
            }
            if (subObj.contains("healthValue")) {
                sub->setHealthValue(subObj["healthValue"].toDouble(100.0));
            }
            
            comp->addSubComponent(sub);   // sets parent
            sub->setPos(sx, sy);          // in parent coords
        }
        
        // Update counter
        if (id.startsWith("component_")) {
            int num = id.mid(10).toInt();
            if (num >= m_componentCounter) {
                m_componentCounter = num;
            }
        }
    }
}
