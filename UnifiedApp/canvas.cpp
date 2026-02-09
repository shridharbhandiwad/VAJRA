#include "canvas.h"
#include "componentregistry.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

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
    
    // Modern dark background
    setBackgroundBrush(QBrush(QColor(24, 26, 31)));
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
    m_scene->clear();
    m_componentCounter = 0;
    m_componentMap.clear();
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
        
        componentsArray.append(compObj);
    }
    
    QJsonObject root;
    root["components"] = componentsArray;
    
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
            // Still allow loading unregistered types for forward compatibility
            typeId = typeStr;
        }
        
        Component* comp = Component::fromJson(id, typeId, x, y, QColor(colorStr), size);
        m_scene->addItem(comp);
        m_componentMap[id] = comp;
        
        qDebug() << "[Canvas] Loaded component" << id << "of type" << typeId 
                 << "at (" << x << "," << y << ")";
        
        emit componentLoaded(id, typeId);
        
        // Update counter
        if (id.startsWith("component_")) {
            int num = id.mid(10).toInt();
            if (num >= m_componentCounter) {
                m_componentCounter = num;
            }
        }
    }
    
    qDebug() << "[Canvas] loadFromJson complete. Total components loaded:" << m_componentMap.size();
}
