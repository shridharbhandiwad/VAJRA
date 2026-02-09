#ifndef COMPONENTREGISTRY_H
#define COMPONENTREGISTRY_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>

/**
 * ComponentDefinition - Data class holding all metadata for a component type.
 * This is the core of the modular architecture: new components are defined
 * entirely through data (JSON), not code.
 */
struct ComponentDefinition {
    QString typeId;          // Unique identifier (e.g., "Antenna", "PowerSystem")
    QString displayName;     // Human-readable name (e.g., "Power System")
    QString label;           // Short label for canvas display (e.g., "PWR")
    QString description;     // Longer description
    QString imageDir;        // Directory name under assets/subsystems/
    QColor iconColor;        // Fallback color when no image is available
    QStringList subsystems;  // Sub-component names for health tracking
    QString protocol;        // Health protocol: "TCP", "UDP", "WebSocket", "MQTT"
    int port;                // Port number for health data
    QString category;        // Grouping category (e.g., "Sensor", "Infrastructure")
    QString shape;           // Fallback geometric shape: "ellipse", "rect", "hexagon", "diamond"
    QStringList allowedWidgets; // Widget sub-component types allowed: "Label", "LineEdit", "Button"

    ComponentDefinition()
        : iconColor(Qt::blue)
        , protocol("TCP")
        , port(12345)
        , shape("rect")
        , allowedWidgets({"Label", "LineEdit", "Button"})
    {}

    QJsonObject toJson() const;
    static ComponentDefinition fromJson(const QJsonObject& obj);
    
    // Helper to get full image path
    QString imagePath() const;
    QString imagePathPng() const;
};

/**
 * ComponentRegistry - Singleton registry that manages all available component types.
 * 
 * Components are loaded from a JSON configuration file (components.json).
 * New components can be added at runtime through the UI, and the registry
 * persists changes back to the JSON file.
 * 
 * This enables a fully modular architecture where NO backend code changes
 * are needed to add new component types.
 */
class ComponentRegistry : public QObject
{
    Q_OBJECT

public:
    static ComponentRegistry& instance();

    // Load/Save
    bool loadFromFile(const QString& filePath = QString());
    bool saveToFile(const QString& filePath = QString()) const;
    
    // Component type management
    bool registerComponent(const ComponentDefinition& def);
    bool unregisterComponent(const QString& typeId);
    bool hasComponent(const QString& typeId) const;
    
    // Accessors
    ComponentDefinition getComponent(const QString& typeId) const;
    QStringList getTypeIds() const;
    QList<ComponentDefinition> getAllComponents() const;
    QStringList getCategories() const;
    QList<ComponentDefinition> getComponentsByCategory(const QString& category) const;
    
    // Protocol info
    QStringList availableProtocols() const;
    
    // Resolve display name to type ID (for drag-drop compatibility)
    QString resolveTypeId(const QString& displayNameOrTypeId) const;
    
    int componentCount() const { return m_components.size(); }

signals:
    void componentRegistered(const QString& typeId);
    void componentUnregistered(const QString& typeId);
    void registryChanged();

private:
    ComponentRegistry();
    ~ComponentRegistry() = default;
    ComponentRegistry(const ComponentRegistry&) = delete;
    ComponentRegistry& operator=(const ComponentRegistry&) = delete;

    QString findConfigFile() const;
    
    QMap<QString, ComponentDefinition> m_components;
    QString m_configFilePath;
};

#endif // COMPONENTREGISTRY_H
