#include "messageserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QNetworkDatagram>

MessageServer::MessageServer(QObject* parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_udpSocket(nullptr)
{
    connect(m_tcpServer, &QTcpServer::newConnection, this, &MessageServer::onNewConnection);
}

MessageServer::~MessageServer()
{
    stopServer();
}

bool MessageServer::startServer(quint16 port)
{
    if (m_tcpServer->isListening()) {
        return true;
    }
    
    if (!m_tcpServer->listen(QHostAddress::Any, port)) {
        qDebug() << "[MessageServer] Failed to start TCP server:" << m_tcpServer->errorString();
        return false;
    }
    
    qDebug() << "[MessageServer] TCP server started on port" << port;
    
    // Also start UDP on port+1
    startUdpServer(port + 1);
    
    return true;
}

bool MessageServer::startUdpServer(quint16 port)
{
    if (m_udpSocket) {
        return true;
    }
    
    m_udpSocket = new QUdpSocket(this);
    if (!m_udpSocket->bind(QHostAddress::Any, port)) {
        qDebug() << "[MessageServer] Failed to start UDP server:" << m_udpSocket->errorString();
        delete m_udpSocket;
        m_udpSocket = nullptr;
        return false;
    }
    
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &MessageServer::onUdpReadyRead);
    qDebug() << "[MessageServer] UDP server started on port" << port;
    return true;
}

bool MessageServer::isRunning() const
{
    return m_tcpServer->isListening();
}

void MessageServer::stopServer()
{
    foreach (QTcpSocket* client, m_clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    m_clients.clear();
    
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
    
    if (m_udpSocket) {
        m_udpSocket->close();
        delete m_udpSocket;
        m_udpSocket = nullptr;
    }
}

void MessageServer::onNewConnection()
{
    QTcpSocket* client = m_tcpServer->nextPendingConnection();
    m_clients.append(client);
    
    connect(client, &QTcpSocket::readyRead, this, &MessageServer::onReadyRead);
    connect(client, &QTcpSocket::disconnected, this, &MessageServer::onDisconnected);
    
    qDebug() << "[MessageServer] TCP client connected:" << client->peerAddress().toString();
    emit clientConnected();
}

void MessageServer::onReadyRead()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    while (client->canReadLine()) {
        QByteArray data = client->readLine().trimmed();
        if (!data.isEmpty()) {
            parseAndEmitMessage(data);
        }
    }
}

void MessageServer::onDisconnected()
{
    QTcpSocket* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    
    qDebug() << "[MessageServer] TCP client disconnected:" << client->peerAddress().toString();
    m_clients.removeAll(client);
    client->deleteLater();
    
    emit clientDisconnected();
}

void MessageServer::onUdpReadyRead()
{
    while (m_udpSocket && m_udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        QByteArray data = datagram.data().trimmed();
        if (!data.isEmpty()) {
            parseAndEmitMessage(data);
        }
    }
}

void MessageServer::parseAndEmitMessage(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qDebug() << "[MessageServer] Invalid JSON received";
        return;
    }
    
    QJsonObject obj = doc.object();
    QString componentId = obj["component_id"].toString();
    QString color = obj["color"].toString();
    qreal size = obj["size"].toDouble();
    
    if (componentId.isEmpty()) {
        qDebug() << "[MessageServer] Missing component_id in message";
        return;
    }
    
    // ── Check for subsystem-level health update ──
    // Format: { "component_id", "subsystem", "color", "size" }
    if (obj.contains("subsystem")) {
        QString subsystem = obj["subsystem"].toString();
        if (!subsystem.isEmpty() && !color.isEmpty()) {
            qDebug() << "[MessageServer] Subsystem health:" << componentId 
                     << "/" << subsystem << ":" << color << size;
            emit subsystemHealthReceived(componentId, subsystem, color, size);
        }
        return;  // Subsystem-only message, don't emit component-level
    }
    
    // ── Basic component-level health update ──
    if (!color.isEmpty() && size >= 0) {
        qDebug() << "[MessageServer] Health update for" << componentId 
                 << ":" << color << size;
        emit messageReceived(componentId, color, size);
    }
    
    // ── Check for subsystem_health map (bulk subsystem update) ──
    // Format: { ..., "subsystem_health": { "SubName": 95.0, ... } }
    if (obj.contains("subsystem_health")) {
        QJsonObject subHealth = obj["subsystem_health"].toObject();
        for (auto it = subHealth.begin(); it != subHealth.end(); ++it) {
            QString subName = it.key();
            qreal subHealthVal = it.value().toDouble();
            // Determine color from health value
            QString subColor;
            if (subHealthVal >= 90) subColor = "#00FF00";
            else if (subHealthVal >= 70) subColor = "#FFFF00";
            else if (subHealthVal >= 40) subColor = "#FFA500";
            else if (subHealthVal >= 10) subColor = "#FF0000";
            else subColor = "#808080";
            
            emit subsystemHealthReceived(componentId, subName, subColor, subHealthVal);
        }
    }
    
    // ── Check for full APCU telemetry ──
    // Format: { ..., "apcu_telemetry": { ... } }
    if (obj.contains("apcu_telemetry")) {
        QJsonObject telemetry = obj["apcu_telemetry"].toObject();
        qDebug() << "[MessageServer] APCU telemetry received for" << componentId;
        emit telemetryReceived(componentId, telemetry);
    }
}
