#include "messageserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

MessageServer::MessageServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &MessageServer::onNewConnection);
}

MessageServer::~MessageServer()
{
    stopServer();
}

bool MessageServer::startServer(quint16 port)
{
    if (m_server->isListening()) {
        return true;
    }
    
    if (!m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "Failed to start server:" << m_server->errorString();
        return false;
    }
    
    qDebug() << "Server started on port" << port;
    return true;
}

void MessageServer::stopServer()
{
    foreach (QTcpSocket* client, m_clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    m_clients.clear();
    
    if (m_server->isListening()) {
        m_server->close();
    }
}

void MessageServer::onNewConnection()
{
    QTcpSocket* client = m_server->nextPendingConnection();
    m_clients.append(client);
    
    connect(client, &QTcpSocket::readyRead, this, &MessageServer::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &MessageServer::onDisconnected);
    
    qDebug() << "Client connected:" << client->peerAddress().toString();
    emit clientConnected();
}

void MessageServer::onReadyRead()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }
    
    while (client->canReadLine()) {
        QByteArray data = client->readLine().trimmed();
        
        if (data.isEmpty()) {
            continue;
        }
        
        // Parse JSON message
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            qDebug() << "Invalid JSON received";
            continue;
        }
        
        QJsonObject obj = doc.object();
        QString componentId = obj["component_id"].toString();
        QString color = obj["color"].toString();
        qreal size = obj["size"].toDouble();
        
        if (!componentId.isEmpty() && !color.isEmpty() && size > 0) {
            qDebug() << "Message received for" << componentId << ":" << color << size;
            emit messageReceived(componentId, color, size);
        }
    }
}

void MessageServer::onDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) {
        return;
    }
    
    qDebug() << "Client disconnected:" << client->peerAddress().toString();
    m_clients.removeAll(client);
    client->deleteLater();
    
    emit clientDisconnected();
}
