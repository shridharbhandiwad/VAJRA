#include "serialprotocolhandler.h"
#include <QDebug>

#ifdef QT_SERIALPORT_LIB
#include <QSerialPortInfo>
#endif

SerialProtocolHandler::SerialProtocolHandler(const QString& protocolName, QObject* parent)
    : ProtocolHandler(parent)
    , m_protoName(protocolName)
{}

SerialProtocolHandler::~SerialProtocolHandler()
{
    stop();
}

bool SerialProtocolHandler::start()
{
#ifdef QT_SERIALPORT_LIB
    if (m_running) return true;

    QString device = m_config.value("device", "").toString();
    int baud = m_config.value("baud", 115200).toInt();

    if (device.isEmpty()) {
        qWarning() << "[" << m_protoName << "] No device configured.";
        return false;
    }

    m_serial = new QSerialPort(this);
    m_serial->setPortName(device);
    m_serial->setBaudRate(baud);

    // Data bits
    int db = m_config.value("dataBits", 8).toInt();
    m_serial->setDataBits(static_cast<QSerialPort::DataBits>(db));

    // Stop bits
    int sb = m_config.value("stopBits", 1).toInt();
    m_serial->setStopBits(static_cast<QSerialPort::StopBits>(sb));

    // Parity
    QString par = m_config.value("parity", "none").toString().toLower();
    if (par == "even")       m_serial->setParity(QSerialPort::EvenParity);
    else if (par == "odd")   m_serial->setParity(QSerialPort::OddParity);
    else                     m_serial->setParity(QSerialPort::NoParity);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        qWarning() << "[" << m_protoName << "] Cannot open" << device
                   << ":" << m_serial->errorString();
        delete m_serial;
        m_serial = nullptr;
        return false;
    }

    connect(m_serial, &QSerialPort::readyRead, this, &SerialProtocolHandler::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, &SerialProtocolHandler::onError);

    m_running = true;
    qDebug() << "[" << m_protoName << "] Opened" << device << "@" << baud;
    emit connected();
    return true;
#else
    qWarning() << "[" << m_protoName << "] QtSerialPort not available."
               << "Falling back to simulation mode (no data will arrive).";
    m_running = true;
    emit connected();
    return true;
#endif
}

void SerialProtocolHandler::stop()
{
#ifdef QT_SERIALPORT_LIB
    if (m_serial) {
        m_serial->close();
        delete m_serial;
        m_serial = nullptr;
    }
#endif
    if (m_running) {
        m_running = false;
        emit disconnected();
    }
}

bool SerialProtocolHandler::isRunning() const
{
    return m_running;
}

void SerialProtocolHandler::onReadyRead()
{
#ifdef QT_SERIALPORT_LIB
    if (!m_serial) return;
    m_buffer += m_serial->readAll();
    // Emit each newline-terminated frame
    int idx;
    while ((idx = m_buffer.indexOf('\n')) != -1) {
        QByteArray frame = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);
        if (!frame.isEmpty()) emit dataReceived(frame);
    }
#endif
}

void SerialProtocolHandler::onError()
{
#ifdef QT_SERIALPORT_LIB
    if (!m_serial) return;
    qWarning() << "[" << m_protoName << "] Error:" << m_serial->errorString();
#endif
}
