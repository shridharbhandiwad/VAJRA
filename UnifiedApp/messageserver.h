#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QList>
#include <QMap>
#include <QJsonObject>

/**
 * MessageServer - Multi-protocol health message receiver.
 * 
 * Supports receiving health data via:
 * - TCP (default, line-delimited JSON)
 * - UDP (JSON datagrams)
 * 
 * Extended APCU Protocol:
 * - Basic health:     { "component_id", "color", "size" }
 * - Subsystem health: { "component_id", "subsystem", "color", "size" }
 * - Full telemetry:   { "component_id", "color", "size", "subsystem_health": {...}, "apcu_telemetry": {...} }
 * 
 * WebSocket and MQTT can be added by extending this class.
 * The protocol is configurable per component type via the ComponentRegistry.
 */
class MessageServer : public QObject
{
    Q_OBJECT
    
public:
    explicit MessageServer(QObject* parent = nullptr);
    ~MessageServer();
    
    bool startServer(quint16 port = 12345);
    bool startUdpServer(quint16 port = 12346);
    void stopServer();
    bool isRunning() const;
    
signals:
    /** Basic component-level health update. */
    void messageReceived(const QString& componentId, const QString& color, qreal size);
    
    /** Per-subsystem health update (from APCU extended protocol). */
    void subsystemHealthReceived(const QString& componentId, const QString& subsystemName,
                                 const QString& color, qreal health);
    
    /** Full APCU telemetry payload received. */
    void telemetryReceived(const QString& componentId, const QJsonObject& telemetry);
    
    void clientConnected();
    void clientDisconnected();
    
private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onUdpReadyRead();
    
private:
    void parseAndEmitMessage(const QByteArray& data);
    
    QTcpServer* m_tcpServer;
    QUdpSocket* m_udpSocket;
    QList<QTcpSocket*> m_clients;
};

#endif // MESSAGESERVER_H
