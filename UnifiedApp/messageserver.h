#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H

#include <QObject>
#include <QMap>
#include <QJsonObject>
#include "protocolhandler.h"
#include "protocolhandlerfactory.h"

/**
 * MessageServer – Multi-protocol health message dispatcher.
 *
 * Each subsystem (component) is registered with its own ProtocolHandler.
 * Supported protocols: TCP, UDP, RS422, RS232, Modbus, CAN.
 * Protocol handlers are interchangeable at runtime; the rest of the
 * application sees only the same signals regardless of the transport.
 *
 * Default behaviour (backward-compatible):
 *   startServer(port)    – starts a shared TCP handler on <port>
 *   startUdpServer(port) – starts a shared UDP handler on <port>
 *
 * Per-component protocol registration:
 *   registerComponentProtocol(componentId, type, config)
 *   – creates a dedicated handler for that component.
 *   The handler forwards its dataReceived() to the same parseAndEmitMessage()
 *   pipeline, with the component_id injected when it is missing from the frame.
 *
 * JSON message shapes (unchanged from original protocol):
 *   Basic:          { "component_id", "color", "size" }
 *   Single system:  { "component_id", "subsystem", "color", "size" }
 *   Bulk subsystem: { "component_id", "color", "size",
 *                     "subsystem_health": { "Name": 0-100, ... } }
 *   APCU telemetry: { ..., "apcu_telemetry": { ... } }
 *   TRM grid:       { ..., "trm_data": [ { "id", "health", "color", ... }, ... ] }
 */
class MessageServer : public QObject
{
    Q_OBJECT

public:
    explicit MessageServer(QObject* parent = nullptr);
    ~MessageServer();

    // ── Legacy convenience API (backward-compatible) ──────────────
    bool startServer(quint16 port = 12345);
    bool startUdpServer(quint16 port = 12346);
    void stopServer();
    bool isRunning() const;

    // ── Per-component protocol management ─────────────────────────
    /**
     * Register a dedicated protocol handler for a specific component.
     * If a handler already exists for componentId it is replaced.
     * Pass an empty componentId to create a "global" handler whose
     * messages are routed purely by the component_id field in the JSON.
     */
    bool registerComponentProtocol(const QString& componentId,
                                   ProtocolType type,
                                   const QVariantMap& config);

    void unregisterComponentProtocol(const QString& componentId);

    /** Returns the protocol type currently in use for a component,
     *  or TCP if no dedicated handler exists. */
    ProtocolType protocolForComponent(const QString& componentId) const;

    QStringList registeredComponents() const;

signals:
    void messageReceived(const QString& componentId, const QString& color, qreal size);
    void subsystemHealthReceived(const QString& componentId, const QString& subsystemName,
                                 const QString& color, qreal health);
    void telemetryReceived(const QString& componentId, const QJsonObject& telemetry);
    void trmDataReceived(const QString& componentId, const QJsonArray& trmArray);
    void clientConnected();
    void clientDisconnected();

private slots:
    void onHandlerData(const QByteArray& data);
    void onHandlerConnected();
    void onHandlerDisconnected();

private:
    void parseAndEmitMessage(const QByteArray& data);
    void attachHandler(const QString& componentId, ProtocolHandler* handler);

    struct HandlerEntry {
        ProtocolType type;
        ProtocolHandler* handler;
    };

    // "global" key "" holds the legacy shared handlers
    QMap<QString, HandlerEntry> m_handlers;
};

#include <QJsonArray>

#endif // MESSAGESERVER_H
