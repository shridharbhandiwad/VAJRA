#include "UdpReceiver.h"
#include <QNetworkDatagram>

UdpReceiver::UdpReceiver(RadarModel *model, QObject *parent)
    : QObject(parent), m_model(model)
{}

bool UdpReceiver::startListening(quint16 port)
{
    m_socket = new QUdpSocket(this);
    bool ok = m_socket->bind(QHostAddress::AnyIPv4, port,
                              QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (ok)
        connect(m_socket, &QUdpSocket::readyRead, this, &UdpReceiver::onReadyRead);
    emit listeningChanged();
    return ok;
}

void UdpReceiver::stopListening()
{
    if (m_socket) m_socket->close();
    emit listeningChanged();
}

void UdpReceiver::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram dg = m_socket->receiveDatagram();
        if (!dg.isValid()) continue;
        QByteArray data = dg.data();
        if (data.isEmpty()) continue;

        const uchar *p   = reinterpret_cast<const uchar*>(data.constData());
        int          len = data.size();
        quint8       type = p[0];

        // ── 0x10: single element ─────────────────────────────────
        if (type == 0x10 && len >= 10) {
            int    q    = p[1];
            int    elem = quint16(p[2]) | (quint16(p[3]) << 8);
            float  pwr  = decodePower  (readU16(p+4));
            float  tmp  = decodeTemp   (readU16(p+6));
            float  cur  = decodeCurrent(readU16(p+8));
            m_model->setElement(q, elem, pwr, tmp, cur);
            ++m_pktCount;
        }

        // ── 0x11: full quadrant ──────────────────────────────────
        else if (type == 0x11 && len >= 2) {
            int q   = p[1];
            int EPQ = m_model->elementsPerQuadrant();
            int expected = 2 + EPQ * 6;
            if (len < expected) continue;

            QVector<float> powers(EPQ), temps(EPQ), currents(EPQ);
            for (int i = 0; i < EPQ; ++i) {
                const uchar *ep = p + 2 + i * 6;
                powers[i]   = decodePower  (readU16(ep));
                temps[i]    = decodeTemp   (readU16(ep+2));
                currents[i] = decodeCurrent(readU16(ep+4));
            }
            m_model->setQuadrant(q, powers, temps, currents);
            ++m_pktCount;
        }

        // ── 0x12: all quadrants snapshot ─────────────────────────
        else if (type == 0x12) {
            int Q   = m_model->quadrantCount();
            int EPQ = m_model->elementsPerQuadrant();
            int expected = 1 + Q * EPQ * 6;
            if (len < expected) continue;

            QVector<QVector<float>> powers(Q), temps(Q), currents(Q);
            for (int q = 0; q < Q; ++q) {
                powers[q].resize(EPQ);
                temps[q].resize(EPQ);
                currents[q].resize(EPQ);
                for (int i = 0; i < EPQ; ++i) {
                    const uchar *ep = p + 1 + (q * EPQ + i) * 6;
                    powers[q][i]   = decodePower  (readU16(ep));
                    temps[q][i]    = decodeTemp   (readU16(ep+2));
                    currents[q][i] = decodeCurrent(readU16(ep+4));
                }
            }
            m_model->setAllQuadrants(powers, temps, currents);
            ++m_pktCount;
        }
        emit packetCountChanged();
    }
}
