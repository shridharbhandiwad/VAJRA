#pragma once
#include <QObject>
#include <QUdpSocket>
#include "RadarModel.h"

/*
 * UDP Receive Packet Protocol  (multi-parameter)
 * ─────────────────────────────────────────────────────────────────
 * Type 0x10 — Single element update
 *   [0x10][quad:1][elem_lo:1][elem_hi:1]
 *   [pwr_lo:1][pwr_hi:1]          scaled: (dBm + 10) * 100  → uint16
 *   [tmp_lo:1][tmp_hi:1]          scaled: temp_C * 10        → uint16
 *   [cur_lo:1][cur_hi:1]          scaled: current_A * 1000   → uint16
 *   Total = 10 bytes
 *
 * Type 0x11 — Full quadrant update
 *   [0x11][quad:1][EPQ × 6 bytes: pwr(2)+tmp(2)+cur(2)]
 *
 * Type 0x12 — All quadrants snapshot (simulator uses this)
 *   [0x12][Q × EPQ × 6 bytes]
 * ─────────────────────────────────────────────────────────────────
 *
 * Encoding helpers:
 *   power  :  raw = (dBm + 10.0) * 100  → stored as uint16
 *   temp   :  raw = temp_C * 10         → stored as uint16
 *   current:  raw = amps  * 1000        → stored as uint16
 *
 *   Decode:  dBm  = raw / 100.0 - 10.0
 *            temp = raw / 10.0
 *            amps = raw / 1000.0
 */
class UdpReceiver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool listening         READ listening         NOTIFY listeningChanged)
    Q_PROPERTY(int  packetsReceived   READ packetsReceived   NOTIFY packetCountChanged)

public:
    explicit UdpReceiver(RadarModel *model, QObject *parent = nullptr);

    bool startListening(quint16 port);
    void stopListening();

    bool listening()       const { return m_socket && m_socket->state() == QAbstractSocket::BoundState; }
    int  packetsReceived() const { return m_pktCount; }

signals:
    void listeningChanged();
    void packetCountChanged();

private slots:
    void onReadyRead();

private:
    static float decodePower  (quint16 raw) { return raw / 100.0f - 10.0f; }
    static float decodeTemp   (quint16 raw) { return raw / 10.0f; }
    static float decodeCurrent(quint16 raw) { return raw / 1000.0f; }
    static quint16 readU16(const uchar *p)  { return quint16(p[0]) | (quint16(p[1]) << 8); }

    RadarModel *m_model   = nullptr;
    QUdpSocket *m_socket  = nullptr;
    int         m_pktCount = 0;
};
