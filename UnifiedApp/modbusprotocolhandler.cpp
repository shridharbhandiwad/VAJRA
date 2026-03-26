#include "modbusprotocolhandler.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

#ifdef QT_SERIALBUS_LIB
#include <QModbusReply>
#endif

ModbusProtocolHandler::ModbusProtocolHandler(QObject* parent)
    : ProtocolHandler(parent)
    , m_pollTimer(new QTimer(this))
{
    connect(m_pollTimer, &QTimer::timeout, this, &ModbusProtocolHandler::poll);
}

ModbusProtocolHandler::~ModbusProtocolHandler()
{
    stop();
}

bool ModbusProtocolHandler::start()
{
    if (m_running) return true;

#ifdef QT_SERIALBUS_LIB
    QString host = m_config.value("host", "localhost").toString();
    int port = m_config.value("port", 502).toInt();

    m_client = new QModbusTcpClient(this);
    m_client->setConnectionParameter(QModbusDevice::NetworkAddressParameter, host);
    m_client->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);

    if (!m_client->connectDevice()) {
        qWarning() << "[Modbus] Failed to connect to" << host << ":" << port;
        delete m_client;
        m_client = nullptr;
        return false;
    }
    qDebug() << "[Modbus] Connected to" << host << ":" << port;
#else
    qWarning() << "[Modbus] QtSerialBus not available. Modbus handler inactive.";
#endif

    int pollMs = m_config.value("poll_ms", 1000).toInt();
    m_pollTimer->start(pollMs);
    m_running = true;
    emit connected();
    return true;
}

void ModbusProtocolHandler::stop()
{
    m_pollTimer->stop();
#ifdef QT_SERIALBUS_LIB
    if (m_client) {
        m_client->disconnectDevice();
        delete m_client;
        m_client = nullptr;
    }
#endif
    if (m_running) {
        m_running = false;
        emit disconnected();
    }
}

void ModbusProtocolHandler::poll()
{
#ifdef QT_SERIALBUS_LIB
    if (!m_client || m_client->state() != QModbusDevice::ConnectedState) return;

    int slaveId = m_config.value("slave_id", 1).toInt();
    QStringList subsystems = m_config.value("subsystems", QStringList()).toStringList();
    // Read registers 0 to 2 + one per subsystem
    int regCount = 3 + subsystems.size();

    QModbusDataUnit req(QModbusDataUnit::HoldingRegisters, 0, regCount);
    QModbusReply* reply = m_client->sendReadRequest(req, slaveId);
    if (!reply) return;

    if (!reply->isFinished()) {
        connect(reply, &QModbusReply::finished, this, &ModbusProtocolHandler::onReplyFinished);
    } else {
        onReplyFinished();
    }
#endif
}

void ModbusProtocolHandler::onReplyFinished()
{
#ifdef QT_SERIALBUS_LIB
    QModbusReply* reply = qobject_cast<QModbusReply*>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QModbusDevice::NoError) {
        qWarning() << "[Modbus] Read error:" << reply->errorString();
        return;
    }

    QModbusDataUnit unit = reply->result();
    QVector<quint16> regs;
    for (int i = 0; i < static_cast<int>(unit.valueCount()); ++i)
        regs.append(unit.value(i));

    QByteArray json = buildJsonFromRegisters(regs);
    if (!json.isEmpty()) emit dataReceived(json);
#endif
}

QByteArray ModbusProtocolHandler::buildJsonFromRegisters(const QVector<quint16>& regs) const
{
    if (regs.size() < 3) return QByteArray();

    QString compId = m_config.value("component_id", "unknown").toString();
    double health = regs[1] / 10.0;
    int statusCode = regs[2];

    static const char* statusColors[] = {
        "#00FF00", "#FFFF00", "#FFA500", "#FF0000", "#808080"
    };
    QString color = statusCode < 5 ? statusColors[statusCode] : "#808080";

    QJsonObject obj;
    obj["component_id"] = compId;
    obj["color"] = color;
    obj["size"] = health;

    QStringList subsystems = m_config.value("subsystems", QStringList()).toStringList();
    if (!subsystems.isEmpty()) {
        QJsonObject subHealth;
        for (int i = 0; i < subsystems.size() && (i + 3) < regs.size(); ++i) {
            subHealth[subsystems[i]] = regs[i + 3] / 10.0;
        }
        obj["subsystem_health"] = subHealth;
    }

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
