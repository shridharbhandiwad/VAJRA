#include "messageserver.h"
#include "tcpprotocolhandler.h"
#include "udpprotocolhandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

// ────────────────────────────────────────────────────────────────────────────
//  Construction / destruction
// ────────────────────────────────────────────────────────────────────────────

MessageServer::MessageServer(QObject* parent)
    : QObject(parent)
{}

MessageServer::~MessageServer()
{
    stopServer();
}

// ────────────────────────────────────────────────────────────────────────────
//  Legacy API (backward-compatible)
// ────────────────────────────────────────────────────────────────────────────

bool MessageServer::startServer(quint16 port)
{
    // Global TCP handler (key "")
    if (m_handlers.contains("") && m_handlers[""].type == ProtocolType::TCP
            && m_handlers[""].handler->isRunning()) {
        return true;
    }

    QVariantMap cfg;
    cfg["port"] = (int)port;

    return registerComponentProtocol("", ProtocolType::TCP, cfg);
}

bool MessageServer::startUdpServer(quint16 port)
{
    // Global UDP handler (key "udp_global")
    const QString key = "udp_global";
    if (m_handlers.contains(key) && m_handlers[key].handler->isRunning())
        return true;

    QVariantMap cfg;
    cfg["port"] = (int)port;

    return registerComponentProtocol(key, ProtocolType::UDP, cfg);
}

void MessageServer::stopServer()
{
    for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it) {
        it->handler->stop();
        it->handler->deleteLater();
    }
    m_handlers.clear();
}

bool MessageServer::isRunning() const
{
    // True if the legacy TCP handler is running
    if (m_handlers.contains("") && m_handlers[""].handler->isRunning())
        return true;
    // Or if any handler is running
    for (auto& e : m_handlers) {
        if (e.handler->isRunning()) return true;
    }
    return false;
}

// ────────────────────────────────────────────────────────────────────────────
//  Per-component protocol management
// ────────────────────────────────────────────────────────────────────────────

bool MessageServer::registerComponentProtocol(const QString& componentId,
                                               ProtocolType type,
                                               const QVariantMap& config)
{
    // Stop & remove existing handler for this component
    if (m_handlers.contains(componentId)) {
        m_handlers[componentId].handler->stop();
        m_handlers[componentId].handler->deleteLater();
        m_handlers.remove(componentId);
    }

    ProtocolHandler* h = ProtocolHandlerFactory::create(type, config, this);
    if (!h) return false;

    attachHandler(componentId, h);

    if (!h->start()) {
        qWarning() << "[MessageServer] Failed to start"
                   << h->protocolName() << "handler for component"
                   << (componentId.isEmpty() ? "(global)" : componentId);
        h->deleteLater();
        m_handlers.remove(componentId);
        return false;
    }

    qDebug() << "[MessageServer] Registered" << h->protocolName()
             << "handler for component"
             << (componentId.isEmpty() ? "(global)" : componentId);
    return true;
}

void MessageServer::unregisterComponentProtocol(const QString& componentId)
{
    if (!m_handlers.contains(componentId)) return;
    m_handlers[componentId].handler->stop();
    m_handlers[componentId].handler->deleteLater();
    m_handlers.remove(componentId);
}

ProtocolType MessageServer::protocolForComponent(const QString& componentId) const
{
    if (m_handlers.contains(componentId))
        return m_handlers[componentId].type;
    return ProtocolType::TCP;
}

QStringList MessageServer::registeredComponents() const
{
    return m_handlers.keys();
}

// ────────────────────────────────────────────────────────────────────────────
//  Internal helpers
// ────────────────────────────────────────────────────────────────────────────

void MessageServer::attachHandler(const QString& componentId, ProtocolHandler* handler)
{
    connect(handler, &ProtocolHandler::dataReceived,
            this, &MessageServer::onHandlerData);
    connect(handler, &ProtocolHandler::connected,
            this, &MessageServer::onHandlerConnected);
    connect(handler, &ProtocolHandler::disconnected,
            this, &MessageServer::onHandlerDisconnected);

    HandlerEntry e;
    e.type = ProtocolHandler::fromString(handler->protocolName());
    e.handler = handler;
    m_handlers[componentId] = e;
}

void MessageServer::onHandlerData(const QByteArray& data)
{
    parseAndEmitMessage(data);
}

void MessageServer::onHandlerConnected()
{
    emit clientConnected();
}

void MessageServer::onHandlerDisconnected()
{
    emit clientDisconnected();
}

// ────────────────────────────────────────────────────────────────────────────
//  Message parsing (unchanged protocol logic)
// ────────────────────────────────────────────────────────────────────────────

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

    // ── Single subsystem update ──────────────────────────────────────
    if (obj.contains("subsystem")) {
        QString subsystem = obj["subsystem"].toString();
        if (!subsystem.isEmpty() && !color.isEmpty()) {
            emit subsystemHealthReceived(componentId, subsystem, color, size);
        }
        return;
    }

    // ── Component-level health update ────────────────────────────────
    if (!color.isEmpty() && size >= 0) {
        emit messageReceived(componentId, color, size);
    }

    // ── Bulk subsystem health map ────────────────────────────────────
    if (obj.contains("subsystem_health")) {
        QJsonObject subHealth = obj["subsystem_health"].toObject();
        for (auto it = subHealth.begin(); it != subHealth.end(); ++it) {
            QString subName = it.key();
            qreal subVal = it.value().toDouble();
            QString subColor;
            if (subVal >= 90)      subColor = "#00FF00";
            else if (subVal >= 70) subColor = "#FFFF00";
            else if (subVal >= 40) subColor = "#FFA500";
            else if (subVal >= 10) subColor = "#FF0000";
            else                   subColor = "#808080";
            emit subsystemHealthReceived(componentId, subName, subColor, subVal);
        }
    }

    // ── TRM grid data ────────────────────────────────────────────────
    if (obj.contains("trm_data")) {
        QJsonArray trmArray = obj["trm_data"].toArray();
        if (!trmArray.isEmpty()) {
            qDebug() << "[MessageServer] TRM data received for" << componentId
                     << "- TRMs:" << trmArray.size();
            emit trmDataReceived(componentId, trmArray);
        }
    }

    // ── Full APCU telemetry ──────────────────────────────────────────
    if (obj.contains("apcu_telemetry")) {
        QJsonObject telemetry = obj["apcu_telemetry"].toObject();
        qDebug() << "[MessageServer] APCU telemetry received for" << componentId;
        emit telemetryReceived(componentId, telemetry);
    }
}
