#ifndef MESSAGESERVER_H
#define MESSAGESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

class MessageServer : public QObject
{
    Q_OBJECT
    
public:
    explicit MessageServer(QObject* parent = nullptr);
    ~MessageServer();
    
    bool startServer(quint16 port = 12345);
    void stopServer();
    bool isRunning() const { return m_server->isListening(); }
    
signals:
    void messageReceived(const QString& componentId, const QString& color, qreal size);
    void clientConnected();
    void clientDisconnected();
    
private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    
private:
    QTcpServer* m_server;
    QList<QTcpSocket*> m_clients;
};

#endif // MESSAGESERVER_H
