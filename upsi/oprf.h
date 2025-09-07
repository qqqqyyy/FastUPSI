#ifndef OPRF_H
#define OPRF_H
#include "ASE/ASE.h"
#include "ASE/poly.h"
#include "rbokvs/rb_okvs.h"
#include "vole.h"

namespace upsi {

template<typename ASEType>
class OPRF{
    void sender(const std::vector<Element>& input, ASE* ase_diff, VoleSender* vole_sender, 
        OPRFValueVec& values, oc::block ro_seed = oc::ZeroBlock, oc::PRNG* prng = nullptr);

    ASE receiver(const std::vector<Element>& input, ASEType* ase, VoleReceiver* vole_receiver, 
        OPRFValueVec& values, oc::block ro_seed = oc::ZeroBlock, oc::PRNG* prng = nullptr);
};

} //namespace upsi
#endif