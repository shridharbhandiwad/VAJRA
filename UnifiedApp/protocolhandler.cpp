#include "protocolhandler.h"

ProtocolType ProtocolHandler::fromString(const QString& s)
{
    QString lower = s.toLower();
    if (lower == "udp")    return ProtocolType::UDP;
    if (lower == "rs422")  return ProtocolType::RS422;
    if (lower == "rs232")  return ProtocolType::RS232;
    if (lower == "modbus") return ProtocolType::Modbus;
    if (lower == "can")    return ProtocolType::CAN;
    return ProtocolType::TCP; // default
}

QString ProtocolHandler::toString(ProtocolType t)
{
    switch (t) {
    case ProtocolType::UDP:    return "UDP";
    case ProtocolType::RS422:  return "RS422";
    case ProtocolType::RS232:  return "RS232";
    case ProtocolType::Modbus: return "Modbus";
    case ProtocolType::CAN:    return "CAN";
    default:                   return "TCP";
    }
}
