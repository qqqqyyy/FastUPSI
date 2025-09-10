
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
    clp.setDefault("days", 4);
    clp.setDefault("ip", "localhost:5001");


    int party = clp.get<int>("party");
    std::string func = clp.get<std::string>("func");
    int days = clp.get<int>("days");
    std::string ip = clp.get<std::string>("ip");
    
    if(party < 0 || party > 1) throw std::runtime_error("party should be 0 or 1");

    auto chl = oc::cp::asioConnect(ip, party == 1);

    std::cout << func << std::endl;

    std::string fn = "party0.txt";
    if(party == 1) fn = "party1.txt";

    if(func == "tree") {
        std::cout << "[Tree] constructor...\n";
        TreeParty tree_party(party, &chl, days, fn);
        std::cout << "[Tree] setup initial sets...\n";
        tree_party.setup();
        std::cout << "[Tree] setup done.\n\n";
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
        std::cout << "[Adaptive] constructor...\n";
        AdaptiveParty adaptive_party(party, &chl, days, fn);
        adaptive_party.refresh_seeds = true;
        std::cout << "[Adaptive] setup initial sets...\n";
        adaptive_party.setup();
        std::cout << "[Adaptive] setup done.\n\n";
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

    return 0;
}
