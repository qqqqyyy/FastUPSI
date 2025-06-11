#include "upsi/tree/party_zero.h"

#include "absl/memory/memory.h"

#include "upsi/network/connection.h"
// #include "upsi/crypto/ec_point_util.h"
// #include "upsi/crypto/elgamal.h"
#include "upsi/roles.h"
#include "upsi/util/data_util.h"
// #include "upsi/util/elgamal_proto_util.h"
#include "upsi/util/proto_util.h"
#include "upsi/util/status.inc"
#include "upsi/utils.h"

namespace upsi {
namespace tree {

Status PartyZero::Handle(const ClientMessage& msg, MessageSink<ServerMessage>* sink) {
    if (ProtocolFinished()) {
        return InvalidArgumentError("[PartyZero] protocol is already complete");
    } else if (!msg.has_tree_msg()) {
        return InvalidArgumentError("[PartyZero] incorrect message type");
    }
    AddComm(msg);

    if (msg.tree_msg().has_message_i()) {
        std::clog << "[Debug] message i size = " << msg.tree_msg().message_i().ByteSizeLong() << std::endl;
        RETURN_IF_ERROR(SendMessageII(msg.tree_msg().message_i(), sink));
        FinishDay();
    } else {
        return InvalidArgumentError("[PartyZero] received a message of unknown type");
    }

    return OkStatus();
}

Status PartyZero::SendMessageII(
    const TreeMessage::MessageI& req, MessageSink<ServerMessage>* sink
) {
    //TODO
    TreeMessage::MessageII res;
    ServerMessage msg;
    *(msg.mutable_tree_msg()->mutable_message_ii()) = res;
    return sink->Send(msg);
}


void PartyZero::PrintComm() {
    unsigned long long total = 0;
    for (size_t day = 0; day < comm_.size(); day++) {
        std::cout << "[PartyZero] Day " << std::to_string(day + 1) << " Comm (B):\t";
        std::cout << comm_[day] << std::endl;
        total += comm_[day];
    }
    std::cout << "[PartyZero] Total Comm (B):\t" << total << std::endl;
}

void PartyZero::PrintResult() {
    std::cout << "[PartyZero] CARDINALITY = " << this->intersection.size() << std::endl;
    if (this->intersection.size() < 30) {
        for (const std::string& element : this->intersection) {
            std::cout << "            " << element << std::endl;
        }
    }
}

}  // namespace tree
}  // namespace upsi
