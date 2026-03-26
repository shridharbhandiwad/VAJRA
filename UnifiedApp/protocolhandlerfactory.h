#ifndef PROTOCOLHANDLERFACTORY_H
#define PROTOCOLHANDLERFACTORY_H

#include "protocolhandler.h"
#include <QVariantMap>

/**
 * ProtocolHandlerFactory – Creates the concrete ProtocolHandler for a given
 * ProtocolType and configuration map.
 */
class ProtocolHandlerFactory
{
public:
    /**
     * Create a handler for the given protocol type.
     * The returned object is owned by the caller (parent must be set by caller).
     */
    static ProtocolHandler* create(ProtocolType type,
                                   const QVariantMap& config,
                                   QObject* parent = nullptr);
};

#endif // PROTOCOLHANDLERFACTORY_H
