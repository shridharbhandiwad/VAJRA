#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QList>
#include <QMap>
#include "component.h"
#include "connection.h"
#include "designsubcomponent.h"

/**
 * CanvasMode - Defines the current interaction mode of the canvas.
 */
enum class CanvasMode {
    Select,       // Default: select and move components
    Connect       // Draw connections between components
};

/**
 * Canvas - The main design/runtime canvas for placing and viewing components.
 * 
 * MODULAR DESIGN: The canvas no longer knows about specific component types.
 * It uses the ComponentRegistry to resolve drag-drop type names to type IDs.
 * Any component type registered in the registry can be placed on the canvas.
 * 
 * CONNECTIONS: Supports drawing uni-directional and bi-directional connections
 * between components with text labels.
 */
class Canvas : public QGraphicsView
{
    Q_OBJECT
    
public:
    explicit Canvas(QWidget* parent = nullptr);
    ~Canvas();
    
    void addComponent(const QString& typeId);
    QList<Component*> getComponents() const;
    Component* getComponentById(const QString& id);
    void clearCanvas();
    void loadFromJson(const QString& json);
    QString saveToJson() const;
    
    // Import/Export single components
    bool importComponent(const QString& filePath);
    bool importSubcomponents(const QString& filePath, Component* targetComponent);
    
    // Read-only mode (disables drag-drop, connection drawing, and component editing)
    void setReadOnly(bool readOnly);
    bool isReadOnly() const { return m_readOnly; }
    
    // Connection management
    void setMode(CanvasMode mode);
    CanvasMode getMode() const { return m_mode; }
    void setConnectionType(ConnectionType type) { m_pendingConnectionType = type; }
    void setConnectionLabel(const QString& label) { m_pendingConnectionLabel = label; }
    
    Connection* addConnection(Component* source, Component* target,
                              ConnectionType type = ConnectionType::Unidirectional,
                              const QString& label = QString());
    void removeConnection(Connection* conn);
    QList<Connection*> getConnections() const { return m_connections; }
    void deleteSelectedConnections();
    
signals:
    void componentAdded(const QString& id, const QString& typeId);
    void componentLoaded(const QString& id, const QString& typeId);
    void designSubComponentAdded(const QString& parentId, SubComponentType subType);
    void dropRejected(const QString& reason);
    void modeChanged(CanvasMode mode);
    void connectionAdded(Connection* conn);
    
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    
private:
    void updateAllConnections();
    Component* componentAtPoint(const QPointF& scenePos);
    Component* componentAtScenePos(const QPointF& scenePos) const;
    void drawPendingConnection(const QPointF& from, const QPointF& to);
    
    QGraphicsScene* m_scene;
    bool m_readOnly;
    int m_componentCounter;
    int m_connectionCounter;
    QMap<QString, Component*> m_componentMap;
    QList<Connection*> m_connections;
    
    // Connection drawing state
    CanvasMode m_mode;
    ConnectionType m_pendingConnectionType;
    QString m_pendingConnectionLabel;
    Component* m_connectionSource;
    QGraphicsLineItem* m_pendingLine;
    bool m_isDrawingConnection;
};

#endif // CANVAS_H
