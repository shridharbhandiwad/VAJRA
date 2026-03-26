#include "udpprotocolhandler.h"
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QDebug>

UdpProtocolHandler::UdpProtocolHandler(QObject* parent)
    : ProtocolHandler(parent)
    , m_socket(nullptr)
{}

UdpProtocolHandler::~UdpProtocolHandler()
{
    stop();
}

bool UdpProtocolHandler::start()
{
    if (m_socket) return true;

    quint16 port = static_cast<quint16>(m_config.value("port", 12346).toInt());
    QString host = m_config.value("host", "0.0.0.0").toString();

    QHostAddress addr(host == "0.0.0.0" ? QHostAddress::Any : QHostAddress(host));

    m_socket = new QUdpSocket(this);
    if (!m_socket->bind(addr, port)) {
        qWarning() << "[UDP] Failed to bind on" << host << ":" << port
                   << "-" << m_socket->errorString();
        delete m_socket;
        m_socket = nullptr;
        return false;
    }
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpProtocolHandler::onReadyRead);
    qDebug() << "[UDP] Bound on port" << port;
    emit connected();
    return true;
}

void UdpProtocolHandler::stop()
{
    if (m_socket) {
        m_socket->close();
        delete m_socket;
        m_socket = nullptr;
        emit disconnected();
    }
}

bool UdpProtocolHandler::isRunning() const
{
    return m_socket && m_socket->state() == QAbstractSocket::BoundState;
}

void UdpProtocolHandler::onReadyRead()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QNetworkDatagram dg = m_socket->receiveDatagram();
        QByteArray data = dg.data().trimmed();
        if (!data.isEmpty()) emit dataReceived(data);
    }
}
