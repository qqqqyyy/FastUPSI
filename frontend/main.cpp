
#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Common/CLP.h"
#include "tree_party.h"
#include "adaptive_party.h"

using namespace upsi;


int main(int argc, char** argv)
{
    oc::CLP clp(argc, argv);
    clp.setDefault("party", 0);
    clp.setDefault("func", "tree");
    clp.setDefault("days", 8);
    clp.setDefault("ip", "localhost:5001");
    clp.setDefault("daily_vole", true);

    int party = clp.get<int>("party");
    std::string func = clp.get<std::string>("func");
    int days = clp.get<int>("days");
    std::string ip = clp.get<std::string>("ip");
    bool del = clp.isSet("del");
    bool daily_vole = clp.get<bool>("daily_vole");

    bool LAN = clp.isSet("LAN");
    bool WAN = clp.isSet("WAN");
    if(LAN && WAN) throw std::runtime_error("LAN & WAN");
    int bandwidth = 200;
    if(WAN) bandwidth = clp.get<int>("WAN");
    
    if(party < 0 || party > 1) throw std::runtime_error("party should be 0 or 1");

    auto chl = oc::cp::asioConnect(ip, party == 1);

    std::cout << func << std::endl;

    std::string fn = "party0.txt";
    if(party == 1) fn = "party1.txt";

    if(func == "tree") {
        std::cout << "[Tree] constructor...\n";
        TreeParty tree_party(party, &chl, days, fn, del, daily_vole);
        std::cout << "[Tree] setup initial sets...\n";
        tree_party.setup();
        std::cout << "[Tree] setup done.\n\n";

        if(LAN && party == 0) std::system("./../network_setup.sh on 0.2 1000");
        if(WAN && party == 0) {
            std::string cmd = "./../network_setup.sh on 40 " + std::to_string(bandwidth);
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

        tree_party.run();
        oc::cp::sync_wait(chl.close());
    }
    else if(func == "adaptive") {
        if(del) throw std::runtime_error("deletion for adaptive protocol not implemented");

        std::cout << "[Adaptive] constructor...\n";
        AdaptiveParty adaptive_party(party, &chl, days, fn, daily_vole);
        adaptive_party.refresh_seeds = true;
        std::cout << "[Adaptive] setup initial sets...\n";
        adaptive_party.setup();
        std::cout << "[Adaptive] setup done.\n\n";

        if(LAN && party == 0) std::system("./../network_setup.sh on 0.2 1000");
        if(WAN && party == 0) {
            std::string cmd = "./../network_setup.sh on 40 " + std::to_string(bandwidth);
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

        adaptive_party.run();
        oc::cp::sync_wait(chl.close());
    }
    else throw std::runtime_error("functionality error");

    if((LAN || WAN) && party == 0) std::system("./../network_setup.sh off");

    return 0;
}
