#include "canprotocolhandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#ifdef QT_SERIALBUS_LIB
#include <QCanBus>
#endif

CanProtocolHandler::CanProtocolHandler(QObject* parent)
    : ProtocolHandler(parent)
{}

CanProtocolHandler::~CanProtocolHandler()
{
    stop();
}

bool CanProtocolHandler::start()
{
    if (m_running) return true;

#ifdef QT_SERIALBUS_LIB
    QString iface = m_config.value("interface", "vcan0").toString();
    QString plugin = m_config.value("plugin", "socketcan").toString();

    QString errorStr;
    m_device = QCanBus::instance()->createDevice(plugin, iface, &errorStr);
    if (!m_device) {
        qWarning() << "[CAN] Cannot create device:" << errorStr;
        return false;
    }

    connect(m_device, &QCanBusDevice::framesReceived,
            this, &CanProtocolHandler::onFramesReceived);

    if (!m_device->connectDevice()) {
        qWarning() << "[CAN] Cannot connect to" << iface
                   << ":" << m_device->errorString();
        delete m_device;
        m_device = nullptr;
        return false;
    }

    qDebug() << "[CAN] Connected to" << iface << "via" << plugin;
    m_running = true;
    emit connected();
    return true;
#else
    qWarning() << "[CAN] QtSerialBus not available. CAN handler inactive.";
    m_running = true;
    emit connected();
    return true;
#endif
}

void CanProtocolHandler::stop()
{
#ifdef QT_SERIALBUS_LIB
    if (m_device) {
        m_device->disconnectDevice();
        delete m_device;
        m_device = nullptr;
    }
#endif
    if (m_running) {
        m_running = false;
        emit disconnected();
    }
}

void CanProtocolHandler::onFramesReceived()
{
#ifdef QT_SERIALBUS_LIB
    if (!m_device) return;

    QString compId = m_config.value("component_id", "unknown").toString();
    QStringList subsystems = m_config.value("subsystems", QStringList()).toStringList();

    static double overallHealth = 100.0;

    while (m_device->framesAvailable()) {
        QCanBusFrame frame = m_device->readFrame();
        quint32 fid = frame.frameId();
        QByteArray payload = frame.payload();

        if (fid == 0x100 && payload.size() >= 1) {
            overallHealth = static_cast<quint8>(payload[0]);
        } else if (fid >= 0x101 && fid <= 0x1FF && payload.size() >= 2) {
            int idx = static_cast<quint8>(payload[0]);
            double health = static_cast<quint8>(payload[1]);
            m_subsystemHealth[idx] = health;
        }
    }

    // Derive color from overall health
    auto healthColor = [](double h) -> QString {
        if (h >= 90) return "#00FF00";
        if (h >= 70) return "#FFFF00";
        if (h >= 40) return "#FFA500";
        if (h >= 10) return "#FF0000";
        return "#808080";
    };

    QJsonObject obj;
    obj["component_id"] = compId;
    obj["color"] = healthColor(overallHealth);
    obj["size"] = overallHealth;

    if (!subsystems.isEmpty() && !m_subsystemHealth.isEmpty()) {
        QJsonObject subH;
        for (int i = 0; i < subsystems.size(); ++i) {
            if (m_subsystemHealth.contains(i))
                subH[subsystems[i]] = m_subsystemHealth[i];
        }
        if (!subH.isEmpty()) obj["subsystem_health"] = subH;
    }

    emit dataReceived(QJsonDocument(obj).toJson(QJsonDocument::Compact));
#endif
}
