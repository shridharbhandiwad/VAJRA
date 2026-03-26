#ifndef TCPPROTOCOLHANDLER_H
#define TCPPROTOCOLHANDLER_H

#include "protocolhandler.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

/**
 * TcpProtocolHandler - TCP server that accepts any number of client connections.
 * Each line-terminated JSON message emits dataReceived().
 *
 * Config keys:
 *   port  (int, required) – TCP listen port
 *   host  (string, optional) – bind address, default "0.0.0.0"
 */
class TcpProtocolHandler : public ProtocolHandler
{
    Q_OBJECT
public:
    explicit TcpProtocolHandler(QObject* parent = nullptr);
    ~TcpProtocolHandler() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;
    QString protocolName() const override { return "TCP"; }

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpServer* m_server;
    QList<QTcpSocket*> m_clients;
};

#endif // TCPPROTOCOLHANDLER_H
