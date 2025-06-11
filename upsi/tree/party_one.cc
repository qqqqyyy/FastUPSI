#include "upsi/tree/party_one.h"

#include "absl/memory/memory.h"

// #include "upsi/crypto/ec_point_util.h"
// #include "upsi/crypto/elgamal.h"
#include "upsi/roles.h"
// #include "upsi/util/elgamal_proto_util.h"
#include "upsi/util/proto_util.h"
#include "upsi/utils.h"

namespace upsi {
namespace tree {

Status PartyOne::Handle(const ServerMessage& msg, MessageSink<ClientMessage>* sink) {
    return OkStatus();
};

Status PartyOne::Run(Connection* sink) {
    Timer timer("[PartyOne] Daily Comp");
    while (!ProtocolFinished()) {
        Timer day("[PartyOne] Day " + std::to_string(this->current_day) + " Comp");
        timer.lap();
        RETURN_IF_ERROR(SendMessageI(sink));
        ServerMessage message_ii = sink->GetResponse();
        FinishDay();
        timer.stop();
        day.stop();
    }
    timer.print();
    return OkStatus();
}

Status PartyOne::SendMessageI(MessageSink<ClientMessage>* sink) {
    //TODO
    TreeMessage::MessageI msg;
    ClientMessage cm;
    *(cm.mutable_tree_msg()->mutable_message_i()) = msg;
    return sink->Send(cm);
}

}  // namespace tree
}  // namespace upsi
