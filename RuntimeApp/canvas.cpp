#include "canvas.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Canvas::Canvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    
    // Set scene size
    m_scene->setSceneRect(0, 0, 600, 500);
    
    // Set background
    setBackgroundBrush(QBrush(QColor(240, 240, 240)));
}

Canvas::~Canvas()
{
}

QList<Component*> Canvas::getComponents() const
{
    return m_componentMap.values();
}

Component* Canvas::getComponentById(const QString& id)
{
    return m_componentMap.value(id, nullptr);
}

void Canvas::clearCanvas()
{
    m_scene->clear();
    m_componentMap.clear();
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
        m_componentMap[id] = comp;
        
        emit componentLoaded(id, typeStr);
    }
}
