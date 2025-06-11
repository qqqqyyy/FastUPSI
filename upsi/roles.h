#pragma once

// #include "upsi/crypto/context.h"
// #include "upsi/crypto/ec_group.h"
// #include "upsi/crypto_tree.h"
#include "upsi/network/connection.h"
#include "upsi/network/message_sink.h"
#include "upsi/params.h"
#include "upsi/network/upsi.pb.h"
#include "upsi/util/proto_util.h"
#include "upsi/util/status.inc"
#include "upsi/utils.h"

namespace upsi {

class ProtocolRole {
    protected:
        int total_days;

        // this is volatile to keep the server from hanging once the protocol is finished
        volatile int current_day = 0;
    public:
        ProtocolRole(PSIParams* params) : total_days(params->total_days) { }

        // call once the day is finished for this party
        virtual void FinishDay() {
            this->current_day++;
        }

        // protocol is finished when we've gone through all days
        virtual bool ProtocolFinished() {
            return (this->current_day >= this->total_days);
        }
};

class Server : public ProtocolRole {
    public:
        using ProtocolRole::ProtocolRole;

        // method called to handle incoming requests
        virtual Status Handle(const ClientMessage& msg, MessageSink<ServerMessage>* sink) = 0;

        // method called at the end of the protocol
        virtual void PrintResult() { }
};

class Client : public ProtocolRole {
    public:
        using ProtocolRole::ProtocolRole;

        // method called to initialize the protocol
        virtual Status Run(Connection* sink) = 0;

        // method called to process responses
        virtual Status Handle(const ServerMessage& msg, MessageSink<ClientMessage>* sink) = 0;

        // method called at the end of the protocol
        virtual void PrintResult() = 0;
};

}  // namespace upsi
