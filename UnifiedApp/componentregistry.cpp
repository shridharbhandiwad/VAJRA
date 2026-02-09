#include "componentregistry.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>

// ============================================================
// ComponentDefinition
// ============================================================

QJsonObject ComponentDefinition::toJson() const
{
    QJsonObject obj;
    obj["type_id"] = typeId;
    obj["display_name"] = displayName;
    obj["label"] = label;
    obj["description"] = description;
    obj["image_dir"] = imageDir;
    obj["icon_color"] = iconColor.name();
    
    QJsonArray subsArr;
    for (const QString& s : subsystems) {
        subsArr.append(s);
    }
    obj["subsystems"] = subsArr;
    
    obj["protocol"] = protocol;
    obj["port"] = port;
    obj["category"] = category;
    obj["shape"] = shape;
    
    return obj;
}

ComponentDefinition ComponentDefinition::fromJson(const QJsonObject& obj)
{
    ComponentDefinition def;
    def.typeId = obj["type_id"].toString();
    def.displayName = obj["display_name"].toString();
    def.label = obj["label"].toString();
    def.description = obj["description"].toString();
    def.imageDir = obj["image_dir"].toString();
    def.iconColor = QColor(obj["icon_color"].toString());
    
    QJsonArray subsArr = obj["subsystems"].toArray();
    for (const QJsonValue& v : subsArr) {
        def.subsystems.append(v.toString());
    }
    
    def.protocol = obj["protocol"].toString("TCP");
    def.port = obj["port"].toInt(12345);
    def.category = obj["category"].toString("General");
    def.shape = obj["shape"].toString("rect");
    
    return def;
}

QString ComponentDefinition::imagePath() const
{
    return QString("assets/subsystems/%1/%1_main.jpg").arg(imageDir);
}

QString ComponentDefinition::imagePathPng() const
{
    return QString("assets/subsystems/%1/%1_main.png").arg(imageDir);
}

// ============================================================
// ComponentRegistry
// ============================================================

ComponentRegistry::ComponentRegistry()
{
}

ComponentRegistry& ComponentRegistry::instance()
{
    static ComponentRegistry registry;
    return registry;
}

QString ComponentRegistry::findConfigFile() const
{
    // Search multiple possible locations
    QStringList searchPaths;
    searchPaths << "components.json";
    searchPaths << "../components.json";
    searchPaths << "UnifiedApp/components.json";
    searchPaths << "../UnifiedApp/components.json";
    searchPaths << "/workspace/UnifiedApp/components.json";
    
    // Also try app dir
    if (!QCoreApplication::applicationDirPath().isEmpty()) {
        searchPaths << QCoreApplication::applicationDirPath() + "/components.json";
    }
    
    for (const QString& path : searchPaths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
    
    return "components.json"; // default
}

bool ComponentRegistry::loadFromFile(const QString& filePath)
{
    QString path = filePath.isEmpty() ? findConfigFile() : filePath;
    m_configFilePath = path;
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ComponentRegistry] Could not open config file:" << path;
        qWarning() << "[ComponentRegistry] A new file will be created when components are added.";
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "[ComponentRegistry] Invalid JSON in config file:" << path;
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray componentsArray = root["components"].toArray();
    
    m_components.clear();
    
    for (const QJsonValue& val : componentsArray) {
        ComponentDefinition def = ComponentDefinition::fromJson(val.toObject());
        if (!def.typeId.isEmpty()) {
            m_components[def.typeId] = def;
            qDebug() << "[ComponentRegistry] Loaded component type:" << def.typeId 
                     << "(" << def.displayName << ")";
        }
    }
    
    qDebug() << "[ComponentRegistry] Loaded" << m_components.size() << "component types from" << path;
    emit registryChanged();
    return true;
}

bool ComponentRegistry::saveToFile(const QString& filePath) const
{
    QString path = filePath.isEmpty() ? m_configFilePath : filePath;
    if (path.isEmpty()) {
        path = "components.json";
    }
    
    QJsonArray componentsArray;
    for (auto it = m_components.constBegin(); it != m_components.constEnd(); ++it) {
        componentsArray.append(it.value().toJson());
    }
    
    QJsonObject root;
    root["version"] = "2.0";
    root["description"] = "Modular component registry - add new components here without changing code";
    root["components"] = componentsArray;
    
    QJsonDocument doc(root);
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[ComponentRegistry] Could not save config file:" << path;
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "[ComponentRegistry] Saved" << m_components.size() << "component types to" << path;
    return true;
}

bool ComponentRegistry::registerComponent(const ComponentDefinition& def)
{
    if (def.typeId.isEmpty()) {
        qWarning() << "[ComponentRegistry] Cannot register component with empty type_id";
        return false;
    }
    
    bool isNew = !m_components.contains(def.typeId);
    m_components[def.typeId] = def;
    
    qDebug() << "[ComponentRegistry]" << (isNew ? "Registered new" : "Updated") 
             << "component type:" << def.typeId;
    
    emit componentRegistered(def.typeId);
    emit registryChanged();
    return true;
}

bool ComponentRegistry::unregisterComponent(const QString& typeId)
{
    if (!m_components.contains(typeId)) {
        return false;
    }
    
    m_components.remove(typeId);
    emit componentUnregistered(typeId);
    emit registryChanged();
    return true;
}

bool ComponentRegistry::hasComponent(const QString& typeId) const
{
    return m_components.contains(typeId);
}

ComponentDefinition ComponentRegistry::getComponent(const QString& typeId) const
{
    return m_components.value(typeId, ComponentDefinition());
}

QStringList ComponentRegistry::getTypeIds() const
{
    return m_components.keys();
}

QList<ComponentDefinition> ComponentRegistry::getAllComponents() const
{
    return m_components.values();
}

QStringList ComponentRegistry::getCategories() const
{
    QStringList categories;
    for (auto it = m_components.constBegin(); it != m_components.constEnd(); ++it) {
        if (!categories.contains(it.value().category)) {
            categories.append(it.value().category);
        }
    }
    categories.sort();
    return categories;
}

QList<ComponentDefinition> ComponentRegistry::getComponentsByCategory(const QString& category) const
{
    QList<ComponentDefinition> result;
    for (auto it = m_components.constBegin(); it != m_components.constEnd(); ++it) {
        if (it.value().category == category) {
            result.append(it.value());
        }
    }
    return result;
}

QStringList ComponentRegistry::availableProtocols() const
{
    return QStringList() << "TCP" << "UDP" << "WebSocket" << "MQTT";
}

QString ComponentRegistry::resolveTypeId(const QString& displayNameOrTypeId) const
{
    // First check direct match
    if (m_components.contains(displayNameOrTypeId)) {
        return displayNameOrTypeId;
    }
    
    // Then check by display name
    for (auto it = m_components.constBegin(); it != m_components.constEnd(); ++it) {
        if (it.value().displayName == displayNameOrTypeId) {
            return it.key();
        }
    }
    
    return QString(); // not found
}
