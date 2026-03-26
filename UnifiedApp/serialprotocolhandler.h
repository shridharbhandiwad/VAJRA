#ifndef SERIALPROTOCOLHANDLER_H
#define SERIALPROTOCOLHANDLER_H

#include "protocolhandler.h"
#include <QByteArray>

// Forward declaration - QSerialPort is conditionally included
#ifdef QT_SERIALPORT_LIB
#include <QSerialPort>
#endif

/**
 * SerialProtocolHandler - RS422 / RS232 handler using QSerialPort.
 *
 * When QtSerialPort module is unavailable (build environment without the
 * module), this handler falls back to a simulation mode that still accepts
 * connections from the local loopback TCP port so existing tests pass.
 *
 * Config keys:
 *   device   (string) – serial device, e.g. "/dev/ttyUSB0", "COM3"
 *   baud     (int)    – baud rate, default 115200
 *   dataBits (int)    – default 8
 *   stopBits (int)    – default 1
 *   parity   (string) – "none"|"even"|"odd", default "none"
 *   protocol (string) – "RS422" or "RS232" (informational only)
 *
 * Fallback (simulation) config keys:
 *   sim_port (int)    – loopback TCP port used when no serial device found
 */
class SerialProtocolHandler : public ProtocolHandler
{
    Q_OBJECT
public:
    explicit SerialProtocolHandler(const QString& protocolName = "RS422",
                                   QObject* parent = nullptr);
    ~SerialProtocolHandler() override;

    bool start() override;
    void stop() override;
    bool isRunning() const override;
    QString protocolName() const override { return m_protoName; }

private slots:
    void onReadyRead();
    void onError();

private:
    QString m_protoName;
    bool m_running = false;

#ifdef QT_SERIALPORT_LIB
    QSerialPort* m_serial = nullptr;
    QByteArray m_buffer;
#endif
};

#endif // SERIALPROTOCOLHANDLER_H
