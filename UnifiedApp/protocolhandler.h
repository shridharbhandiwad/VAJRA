#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVariantMap>

/**
 * ProtocolType - Enumeration of supported communication protocols.
 * Each subsystem can independently use any protocol.
 */
enum class ProtocolType {
    TCP,
    UDP,
    RS422,
    RS232,
    Modbus,
    CAN
};

/**
 * ProtocolHandler - Abstract base class for all communication protocol handlers.
 *
 * Concrete implementations exist for TCP, UDP, RS422/RS232 (serial),
 * Modbus, and CAN. Each handler opens its transport, receives raw bytes,
 * and emits dataReceived() when a complete frame/datagram is available.
 * MessageServer owns one handler per registered component and connects
 * dataReceived() to its internal parsing pipeline.
 *
 * Subclasses must implement:
 *   - start()  : open the transport
 *   - stop()   : close it cleanly
 *   - isRunning() const
 *   - protocolName() const : human-readable tag for logs
 */
class ProtocolHandler : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolHandler(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ProtocolHandler() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;
    virtual QString protocolName() const = 0;

    /** Configuration parameters (set before start()).
     *  Common keys:
     *    host    – remote host (TCP/Modbus)
     *    port    – port number (TCP/UDP/Modbus)
     *    device  – serial device path (RS422/RS232/CAN)
     *    baud    – baud rate (RS422/RS232)
     *    canIface– CAN interface name
     */
    void setConfig(const QVariantMap& cfg) { m_config = cfg; }
    QVariantMap config() const { return m_config; }

    /** Utility: parse a ProtocolType from a string (case-insensitive). */
    static ProtocolType fromString(const QString& s);
    static QString toString(ProtocolType t);

signals:
    void dataReceived(const QByteArray& data);
    void connected();
    void disconnected();

protected:
    QVariantMap m_config;
};

#endif // PROTOCOLHANDLER_H
