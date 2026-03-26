#include "protocolhandlerfactory.h"
#include "tcpprotocolhandler.h"
#include "udpprotocolhandler.h"
#include "serialprotocolhandler.h"
#include "modbusprotocolhandler.h"
#include "canprotocolhandler.h"

ProtocolHandler* ProtocolHandlerFactory::create(ProtocolType type,
                                                 const QVariantMap& config,
                                                 QObject* parent)
{
    ProtocolHandler* handler = nullptr;

    switch (type) {
    case ProtocolType::UDP:
        handler = new UdpProtocolHandler(parent);
        break;
    case ProtocolType::RS422:
        handler = new SerialProtocolHandler("RS422", parent);
        break;
    case ProtocolType::RS232:
        handler = new SerialProtocolHandler("RS232", parent);
        break;
    case ProtocolType::Modbus:
        handler = new ModbusProtocolHandler(parent);
        break;
    case ProtocolType::CAN:
        handler = new CanProtocolHandler(parent);
        break;
    case ProtocolType::TCP:
    default:
        handler = new TcpProtocolHandler(parent);
        break;
    }

    if (handler) handler->setConfig(config);
    return handler;
}
