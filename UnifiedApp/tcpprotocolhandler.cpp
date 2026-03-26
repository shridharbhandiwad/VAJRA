#include "tcpprotocolhandler.h"
#include <QHostAddress>
#include <QDebug>

TcpProtocolHandler::TcpProtocolHandler(QObject* parent)
    : ProtocolHandler(parent)
    , m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection,
            this, &TcpProtocolHandler::onNewConnection);
}

TcpProtocolHandler::~TcpProtocolHandler()
{
    stop();
}

bool TcpProtocolHandler::start()
{
    if (m_server->isListening()) return true;

    quint16 port = static_cast<quint16>(m_config.value("port", 12345).toInt());
    QString host = m_config.value("host", "0.0.0.0").toString();

    QHostAddress addr(host == "0.0.0.0" ? QHostAddress::Any : QHostAddress(host));

    if (!m_server->listen(addr, port)) {
        qWarning() << "[TCP] Failed to listen on" << host << ":" << port
                   << "-" << m_server->errorString();
        return false;
    }
    qDebug() << "[TCP] Listening on port" << port;
    emit connected();
    return true;
}

void TcpProtocolHandler::stop()
{
    for (QTcpSocket* s : m_clients) {
        s->disconnectFromHost();
        s->deleteLater();
    }
    m_clients.clear();

    if (m_server->isListening()) {
        m_server->close();
        emit disconnected();
    }
}

bool TcpProtocolHandler::isRunning() const
{
    return m_server->isListening();
}

void TcpProtocolHandler::onNewConnection()
{
    QTcpSocket* client = m_server->nextPendingConnection();
    if (!client) return;
    m_clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &TcpProtocolHandler::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &TcpProtocolHandler::onDisconnected);
    qDebug() << "[TCP] Client connected:" << client->peerAddress().toString();
}

void TcpProtocolHandler::onReadyRead()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    while (client->canReadLine()) {
        QByteArray line = client->readLine().trimmed();
        if (!line.isEmpty()) emit dataReceived(line);
    }
}

void TcpProtocolHandler::onDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    qDebug() << "[TCP] Client disconnected:" << client->peerAddress().toString();
    m_clients.removeAll(client);
    client->deleteLater();
}
