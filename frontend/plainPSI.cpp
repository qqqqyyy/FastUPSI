
#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Common/CLP.h"
#include "tree_party.h"
#include "adaptive_party.h"

using namespace upsi;


int main(int argc, char** argv)
{
    oc::CLP clp(argc, argv);
    clp.setDefault("party", 0);
    clp.setDefault("ip", "localhost:5001");

    int party = clp.get<int>("party");
    std::string ip = clp.get<std::string>("ip");

    bool LAN = clp.isSet("LAN");
    bool WAN = clp.isSet("WAN");
    if(LAN && WAN) throw std::runtime_error("LAN & WAN");
    int bandwidth = 200;
    if(WAN) bandwidth = clp.get<int>("WAN");
    
    if(party < 0 || party > 1) throw std::runtime_error("party should be 0 or 1");

    auto chl = oc::cp::asioConnect(ip, party == 1);

    std::string fn = "party0.txt";
    if(party == 1) fn = "party1.txt";


    std::cout << "[Party] constructor...\n";
    Party plainPSI_party(party, &chl, 0, fn, false, true);
    // int n = plainPSI_party.dataset.start_size;
    // int vole_size = 1.06 * n + 1;

    if(LAN && party == 0) std::system("./../network_setup.sh on 0.2 1000");
    if(WAN && party == 0) {
        std::string cmd = "./../network_setup.sh on 80 " + std::to_string(bandwidth);
        std::system(cmd.c_str());
    }

    if(party == 0) {
        int tmp = 0;
        oc::cp::sync_wait(chl.send(tmp));
        oc::cp::sync_wait(chl.flush());
        oc::cp::sync_wait(chl.recv(tmp));
    }
    else {
        int tmp = 0;
        oc::cp::sync_wait(chl.recv(tmp));
        oc::cp::sync_wait(chl.send(tmp));
        oc::cp::sync_wait(chl.flush());
    }
    size_t last_comm = chl.bytesSent() + chl.bytesReceived();
    oc::Timer t0("plain psi");
    t0.setTimePoint("begin");
    if(party == 0) {
        // plainPSI_party.vole_receiver.n = 1 << 16;
        // plainPSI_party.vole_receiver.m = plainPSI_party.vole_receiver.n - 128;
        // plainPSI_party.vole_receiver.setup(&chl, &plainPSI_party.my_prng);
        // plainPSI_party.vole_receiver.has_base = false;
        // plainPSI_party.vole_receiver.generate(vole_size);
        std::cout << (plainPSI_party.PSI_receiver(plainPSI_party.dataset.initial_set)).size() <<"\n";
    }
    else {
        // plainPSI_party.vole_sender.n = 1 << 16;
        // plainPSI_party.vole_sender.m = plainPSI_party.vole_sender.n - 128;
        // plainPSI_party.vole_sender.setup(&chl, &plainPSI_party.my_prng);
        // plainPSI_party.vole_sender.has_base = false;
        // plainPSI_party.vole_sender.generate(vole_size);
        plainPSI_party.PSI_sender(plainPSI_party.dataset.initial_set);
    }

    t0.setTimePoint("end");
    std::cout << t0 << "\n";
    size_t cur_comm = chl.bytesSent() + chl.bytesReceived() - last_comm;
    std::cout << "[Comm.(MB)] (both parties) total = " << cur_comm / 1024.0 / 1024.0 << "\n";

    oc::cp::sync_wait(chl.close());

    if((LAN || WAN) && party == 0) std::system("./../network_setup.sh off");

    return 0;
}
