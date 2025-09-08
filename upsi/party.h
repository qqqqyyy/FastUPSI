#ifndef PARTY_H
#define PARTY_H
#include "utils.h"
#include "ASE/ASE.h"
#include "vole.h"

namespace upsi{

class Party{

    public: 

        int total_days;
        int current_day = 0;
        int max_data_size;
        int party; // 0 / 1

        VoleSender vole_sender;
        VoleReceiver vole_receiver;

        OPRF oprf;
        Socket* chl;
        PRNG my_prng;

        std::vector<Element> intersection;

        Party(int _party, Socket* _chl, int _total_days, int _max_data_size):
                party(_party), chl(_chl), total_days(_total_days), max_data_size(_max_data_size) {
            
            my_prng.SetSeed(oc::sysRandomSeed());
            vole_sender.setup(chl, my_prng);
            vole_receiver.setup(chl, my_prng);
            
            // TODO: max_vole_size?
            size_t max_vole_size = max_data_size * (4 * (log2ceil(max_data_size) + 1) + 2) + 500 * (total_days + 1);
            if(party == 0) {
                vole_sender.generate(max_vole_size);
                vole_receiver.generate(max_vole_size);
            }
            else {
                vole_receiver.generate(max_vole_size);
                vole_sender.generate(max_vole_size);
            }
        }

};

}
#endif