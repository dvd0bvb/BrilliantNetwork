#pragma once

#include "AsioIncludes.h"
#include "ConnectionProcess.h"

namespace Brilliant
{
    namespace Network
    {
        class AcceptorInterface;

        class ServerProcess : public ConnectionProcess
        {
        public:
            virtual void OnAcceptorError(AcceptorInterface& acceptor, const error_code& ec) = 0;
            virtual void OnAcceptedConnection(ConnectionInterface& connection) = 0;
            virtual void OnBeginListen(ConnectionInterface& listener) = 0;
            virtual void OnAcceptorClose(AcceptorInterface& acceptor) = 0;
        };
    }
}