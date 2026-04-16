#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>
#include "AntennaConfig.h"

/*
 * Command UDP Packet Protocol
 * ─────────────────────────────────────────────────────
 * All commands are 6 bytes:
 *   [0xC0][cmd][quad 0-3][elem_lo][elem_hi][checksum]
 *
 *   cmd:
 *     0x01 = POWER_OFF
 *     0x02 = CALIBRATE
 *     0x03 = RESTART
 *
 *   elem_lo / elem_hi = element index (little-endian uint16)
 *   checksum = XOR of bytes 0-4
 *
 * Response (ACK) comes back on ackPort = cmdPort + 1:
 *   [0xCA][cmd][quad][elem_lo][elem_hi][status]
 *   status: 0x00=OK, 0x01=BUSY, 0x02=ERROR
 * ─────────────────────────────────────────────────────
 */

class CommandSender : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantList commandLog READ commandLog NOTIFY commandLogChanged)
    Q_PROPERTY(int          pendingCount READ pendingCount NOTIFY pendingCountChanged)

public:
    enum Command { PowerOff = 0x01, Calibrate = 0x02, Restart = 0x03 };
    Q_ENUM(Command)

    enum Status { Pending, Ack_OK, Ack_Busy, Ack_Error, Timeout };
    Q_ENUM(Status)

    explicit CommandSender(const AntennaConfig &cfg, QObject *parent = nullptr);

    Q_INVOKABLE bool sendCommand(int command, int quadrant, int elementId);
    Q_INVOKABLE void clearLog();

    QVariantList commandLog()  const { return m_log; }
    int          pendingCount()const { return m_pending; }

signals:
    void commandLogChanged();
    void pendingCountChanged();
    void commandAcknowledged(int command, int quadrant, int elementId, int status);

private slots:
    void onAckReceived();

private:
    static constexpr int MAX_LOG = 100;

    void appendLog(int cmd, int quad, int elem, int status, const QString &ts);

    AntennaConfig  m_cfg;
    QUdpSocket    *m_txSocket  = nullptr;
    QUdpSocket    *m_ackSocket = nullptr;
    QVariantList   m_log;
    int            m_pending   = 0;
};
