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
    Component* comp = m_componentMap.value(id, nullptr);
    qDebug() << "[Canvas] getComponentById(" << id << ") =" << (comp ? "FOUND" : "NOT FOUND");
    qDebug() << "[Canvas] Current map size:" << m_componentMap.size();
    qDebug() << "[Canvas] Map keys:" << m_componentMap.keys();
    return comp;
}

void Canvas::clearCanvas()
{
    m_scene->clear();
    m_componentMap.clear();
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
    
    foreach (const QJsonValue& value, componentsArray) {
        QJsonObject compObj = value.toObject();
        
        QString id = compObj["id"].toString();
        QString typeStr = compObj["type"].toString();
        qreal x = compObj["x"].toDouble();
        qreal y = compObj["y"].toDouble();
        QString colorStr = compObj["color"].toString();
        qreal size = compObj["size"].toDouble();
        
        ComponentType type;
        if (typeStr == "Antenna") type = ComponentType::Antenna;
        else if (typeStr == "PowerSystem") type = ComponentType::PowerSystem;
        else if (typeStr == "LiquidCoolingUnit") type = ComponentType::LiquidCoolingUnit;
        else if (typeStr == "CommunicationSystem") type = ComponentType::CommunicationSystem;
        else if (typeStr == "RadarComputer") type = ComponentType::RadarComputer;
        else {
            qDebug() << "[Canvas] Unknown component type:" << typeStr;
            continue;
        }
        
        Component* comp = Component::fromJson(id, type, x, y, QColor(colorStr), size);
        m_scene->addItem(comp);
        m_componentMap[id] = comp;
        
        qDebug() << "[Canvas] Loaded component" << id << "of type" << typeStr << "at (" << x << "," << y << ")";
        qDebug() << "[Canvas] Component map now contains" << m_componentMap.size() << "components";
        
        emit componentLoaded(id, typeStr);
    }
    
    qDebug() << "[Canvas] loadFromJson complete. Total components loaded:" << m_componentMap.size();
}
