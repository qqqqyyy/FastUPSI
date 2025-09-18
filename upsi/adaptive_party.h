#ifndef ADAPTIVE_PARTY_H
#define ADAPTIVE_PARTY_H

#include "party.h"
#include "adaptive.h"
#include "oprf.h"

namespace upsi{

class AdaptiveParty : public Party{
    public: 
        Adaptive<PlainASE> my_adaptive;
        Adaptive<rb_okvs> other_adaptive;


        AdaptiveParty(int _party, oc::Socket* _chl, int _total_days, std::string fn, bool daily_vole = false):
                Party(_party, _chl, _total_days, fn, false, daily_vole){
            if(party == 0) {
                ro_seed = oc::sysRandomSeed();
                coproto::sync_wait(chl->send(ro_seed));
            }
            else {
                coproto::sync_wait(chl->recv(ro_seed));
            }
            my_adaptive.setup(DEFAULT_ADAPTIVE_SIZE);
            other_adaptive.setup(DEFAULT_ADAPTIVE_SIZE);
        }


        // void setup() override{
        //     addition(dataset.initial_set);
        //     for (const auto& cur_elem: dataset.intersection) intersection[cur_elem] = true;
        //     my_adaptive.addASE();
        //     other_adaptive.addASE();
        //     std::cout << "[VOLE] setup used: " << vole_receiver.idx << "\n";
        // }


        std::vector<Element> query(const std::vector<Element>& elems) override; // query for elems
        
        void addition(const std::vector<Element>& elems) override;
        
};

}

#endif