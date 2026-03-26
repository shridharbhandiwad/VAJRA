#ifndef UDPPROTOCOLHANDLER_H
#define UDPPROTOCOLHANDLER_H

#include "protocolhandler.h"
#include <QUdpSocket>

/**
 * UdpProtocolHandler - UDP datagram receiver.
 * Each received datagram is emitted as dataReceived().
 *
 * Config keys:
 *   port  (int, required) – UDP bind port
 *   host  (string, optional) – bind address, default "0.0.0.0"
 */
class UdpProtocolHandler : public ProtocolHandler
{
    Q_OBJECT
public:
    explicit UdpProtocolHandler(QObject* parent = nullptr);
    ~UdpProtocolHandler() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;
    QString protocolName() const override { return "UDP"; }

private slots:
    void onReadyRead();

private:
    QUdpSocket* m_socket;
};

#endif // UDPPROTOCOLHANDLER_H
