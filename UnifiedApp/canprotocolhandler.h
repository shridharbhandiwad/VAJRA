#ifndef CANPROTOCOLHANDLER_H
#define CANPROTOCOLHANDLER_H

#include "protocolhandler.h"
#include <QTimer>

#ifdef QT_SERIALBUS_LIB
#include <QCanBusDevice>
#include <QCanBusFrame>
#endif

/**
 * CanProtocolHandler – CAN bus receiver (SocketCAN / Peak / Vector).
 *
 * Receives CAN frames and maps them to the standard JSON health message
 * format.  Frame layout:
 *
 *   Frame ID 0x100 – overall health byte (0-100), health status nibble
 *   Frame ID 0x101…0x1FF – per-subsystem health (data byte 0 = index,
 *                           data byte 1 = health value 0-100)
 *
 * Config keys:
 *   interface   (string) – CAN interface, e.g. "vcan0", "can0"
 *   plugin      (string) – Qt CAN plugin name, e.g. "socketcan", default "socketcan"
 *   component_id (string) – injected into emitted JSON
 *   subsystems  (QStringList) – subsystem names in order
 *
 * When QtSerialBus is unavailable the handler falls back to inactive mode.
 */
class CanProtocolHandler : public ProtocolHandler
{
    Q_OBJECT
public:
    explicit CanProtocolHandler(QObject* parent = nullptr);
    ~CanProtocolHandler() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override { return m_running; }
    QString protocolName() const override { return "CAN"; }

private slots:
    void onFramesReceived();

private:
    bool m_running = false;

#ifdef QT_SERIALBUS_LIB
    QCanBusDevice* m_device = nullptr;
    QMap<int, double> m_subsystemHealth;
#endif
};

#endif // CANPROTOCOLHANDLER_H
