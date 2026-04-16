#include "CommandSender.h"
#include <QHostAddress>
#include <QNetworkDatagram>

CommandSender::CommandSender(const AntennaConfig &cfg, QObject *parent)
    : QObject(parent), m_cfg(cfg)
{
    m_txSocket  = new QUdpSocket(this);
    m_ackSocket = new QUdpSocket(this);

    // Listen for ACKs on udpPort + 1
    m_ackSocket->bind(QHostAddress::AnyIPv4,
                      static_cast<quint16>(m_cfg.udpPort + 1),
                      QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(m_ackSocket, &QUdpSocket::readyRead, this, &CommandSender::onAckReceived);
}

bool CommandSender::sendCommand(int command, int quadrant, int elementId)
{
    if (quadrant < 0 || quadrant >= m_cfg.quadrants)      return false;
    if (elementId < 0 || elementId >= m_cfg.elementsPerQuadrant) return false;
    if (command < 1 || command > 3)                        return false;

    // Build 6-byte command packet
    quint8 elemLo = static_cast<quint8>(elementId & 0xFF);
    quint8 elemHi = static_cast<quint8>((elementId >> 8) & 0xFF);
    QByteArray pkt(6, 0);
    pkt[0] = static_cast<char>(0xC0);
    pkt[1] = static_cast<char>(command);
    pkt[2] = static_cast<char>(quadrant);
    pkt[3] = static_cast<char>(elemLo);
    pkt[4] = static_cast<char>(elemHi);
    // Checksum = XOR of bytes 0-4
    quint8 cs = 0;
    for (int i = 0; i < 5; ++i) cs ^= static_cast<quint8>(pkt[i]);
    pkt[5] = static_cast<char>(cs);

    qint64 sent = m_txSocket->writeDatagram(pkt, QHostAddress::LocalHost,
                                             static_cast<quint16>(m_cfg.udpPort));
    if (sent != pkt.size()) return false;

    ++m_pending;
    emit pendingCountChanged();

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    appendLog(command, quadrant, elementId, static_cast<int>(Pending), ts);
    return true;
}

void CommandSender::onAckReceived()
{
    while (m_ackSocket->hasPendingDatagrams()) {
        QNetworkDatagram dg = m_ackSocket->receiveDatagram(16);
        if (!dg.isValid()) continue;
        QByteArray data = dg.data();
        if (data.size() < 6) continue;
        if (static_cast<quint8>(data[0]) != 0xCA) continue;

        int cmd    = static_cast<quint8>(data[1]);
        int quad   = static_cast<quint8>(data[2]);
        int elemLo = static_cast<quint8>(data[3]);
        int elemHi = static_cast<quint8>(data[4]);
        int elem   = elemLo | (elemHi << 8);
        int status = static_cast<quint8>(data[5]); // 0=OK,1=BUSY,2=ERROR

        if (m_pending > 0) { --m_pending; emit pendingCountChanged(); }

        QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        int logStatus = (status == 0) ? static_cast<int>(Ack_OK)
                      : (status == 1) ? static_cast<int>(Ack_Busy)
                                      : static_cast<int>(Ack_Error);
        appendLog(cmd, quad, elem, logStatus, ts);
        emit commandAcknowledged(cmd, quad, elem, logStatus);
    }
}

void CommandSender::clearLog()
{
    m_log.clear();
    emit commandLogChanged();
}

void CommandSender::appendLog(int cmd, int quad, int elem, int status, const QString &ts)
{
    static const QStringList cmdNames  = { "", "POWER OFF", "CALIBRATE", "RESTART" };
    static const QStringList statNames = { "PENDING", "ACK OK", "ACK BUSY", "ACK ERROR", "TIMEOUT" };

    QVariantMap entry;
    entry["time"]    = ts;
    entry["cmd"]     = cmd;
    entry["cmdName"] = (cmd >= 1 && cmd <= 3) ? cmdNames[cmd] : "UNKNOWN";
    entry["quad"]    = quad;
    entry["elem"]    = quad * m_cfg.elementsPerQuadrant + elem;
    entry["localElem"] = elem;
    entry["status"]  = status;
    entry["statName"]= (status >= 0 && status <= 4) ? statNames[status] : "?";

    m_log.prepend(entry);          // newest first
    if (m_log.size() > MAX_LOG)
        m_log.removeLast();

    emit commandLogChanged();
}
