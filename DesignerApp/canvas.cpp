#include "canvas.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
    QString typeStr = event->mimeData()->text();
    ComponentType type;
    
    if (typeStr == "Circle") {
        type = ComponentType::Circle;
    } else if (typeStr == "Square") {
        type = ComponentType::Square;
    } else if (typeStr == "Triangle") {
        type = ComponentType::Triangle;
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

QString Canvas::saveToJson() const
{
    QJsonArray componentsArray;
    
    foreach (Component* comp, getComponents()) {
        QJsonObject compObj;
        compObj["id"] = comp->getId();
        
        QString typeStr;
        switch (comp->getType()) {
            case ComponentType::Circle: typeStr = "Circle"; break;
            case ComponentType::Square: typeStr = "Square"; break;
            case ComponentType::Triangle: typeStr = "Triangle"; break;
        }
        compObj["type"] = typeStr;
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
    clearCanvas();
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray componentsArray = root["components"].toArray();
    
    foreach (const QJsonValue& value, componentsArray) {
        QJsonObject compObj = value.toObject();
        
        QString id = compObj["id"].toString();
        QString typeStr = compObj["type"].toString();
        qreal x = compObj["x"].toDouble();
        qreal y = compObj["y"].toDouble();
        QString colorStr = compObj["color"].toString();
        qreal size = compObj["size"].toDouble();
        
        ComponentType type;
        if (typeStr == "Circle") type = ComponentType::Circle;
        else if (typeStr == "Square") type = ComponentType::Square;
        else if (typeStr == "Triangle") type = ComponentType::Triangle;
        else continue;
        
        Component* comp = Component::fromJson(id, type, x, y, QColor(colorStr), size);
        m_scene->addItem(comp);
        
        // Update counter
        if (id.startsWith("component_")) {
            int num = id.mid(10).toInt();
            if (num >= m_componentCounter) {
                m_componentCounter = num;
            }
        }
    }
}
