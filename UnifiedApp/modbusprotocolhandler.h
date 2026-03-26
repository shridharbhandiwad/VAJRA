#ifndef MODBUSPROTOCOLHANDLER_H
#define MODBUSPROTOCOLHANDLER_H

#include "protocolhandler.h"
#include <QTimer>
#include <QJsonObject>

// Forward declarations for optional Qt SerialBus module
#ifdef QT_SERIALBUS_LIB
#include <QModbusTcpClient>
#include <QModbusDataUnit>
#endif

/**
 * ModbusProtocolHandler – Modbus TCP/RTU client that periodically polls
 * holding registers and converts the result to the standard JSON health
 * message format.
 *
 * Register layout (configurable, defaults match simulator):
 *   0  – component_id hash (uint16, informational)
 *   1  – overall health   (uint16, 0-1000, divide by 10 → 0.0-100.0)
 *   2  – status code:  0=operational, 1=warning, 2=degraded, 3=critical, 4=offline
 *   3…N – subsystem health values (uint16, 0-1000 each)
 *
 * Config keys:
 *   host        (string) – Modbus server host, default "localhost"
 *   port        (int)    – Modbus TCP port, default 502
 *   slave_id    (int)    – Modbus slave/unit ID, default 1
 *   poll_ms     (int)    – polling interval ms, default 1000
 *   component_id (string) – injected into emitted JSON
 *   subsystems  (QStringList) – names for registers 3…N
 *
 * When QtSerialBus is unavailable, falls back to a direct TCP connection
 * expecting line-delimited JSON on the specified port (same as TCP handler).
 */
class ModbusProtocolHandler : public ProtocolHandler
{
    Q_OBJECT
public:
    explicit ModbusProtocolHandler(QObject* parent = nullptr);
    ~ModbusProtocolHandler() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    QString protocolName() const override { return "Modbus"; }

private slots:
    void poll();
    void onReplyFinished();

private:
    QByteArray buildJsonFromRegisters(const QVector<quint16>& regs) const;

    QTimer* m_pollTimer;
    bool m_running = false;

#ifdef QT_SERIALBUS_LIB
    QModbusTcpClient* m_client = nullptr;
#endif
};

#endif // MODBUSPROTOCOLHANDLER_H
