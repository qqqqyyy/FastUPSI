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

        VoleSender vole_sender;
        VoleReceiver vole_receiver;

};

}
#endif