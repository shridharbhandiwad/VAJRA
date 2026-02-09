#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QList>

/**
 * MessageServer - Multi-protocol health message receiver.
 * 
 * Supports receiving health data via:
 * - TCP (default, line-delimited JSON)
 * - UDP (JSON datagrams)
 * 
 * WebSocket and MQTT can be added by extending this class.
 * The protocol is configurable per component type via the ComponentRegistry.
 */
class MessageServer : public QObject
{
    Q_OBJECT
    
public:
    explicit MessageServer(QObject* parent = nullptr);
    ~MessageServer();
    
    bool startServer(quint16 port = 12345);
    bool startUdpServer(quint16 port = 12346);
    void stopServer();
    bool isRunning() const;
    
signals:
    void messageReceived(const QString& componentId, const QString& color, qreal size);
    void clientConnected();
    void clientDisconnected();
    
private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onUdpReadyRead();
    
private:
    void parseAndEmitMessage(const QByteArray& data);
    
    QTcpServer* m_tcpServer;
    QUdpSocket* m_udpSocket;
    QList<QTcpSocket*> m_clients;
};

#endif // MESSAGESERVER_H
