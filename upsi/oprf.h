#ifndef OPRF_H
#define OPRF_H
#include "ASE/ASE.h"
#include "ASE/poly.h"
#include "rbokvs/rb_okvs.h"
#include "vole.h"

namespace upsi {

template<typename ASEType>
class OPRF{
public:
    void sender(const std::vector<Element>& input, ASEType& b, oc::block delta,
        OPRFValueVec& values, oc::block ro_seed = oc::ZeroBlock);

    void receiver(const std::vector<Element>& input, ASEType& a, 
        OPRFValueVec& values, oc::block ro_seed = oc::ZeroBlock);
};

} //namespace upsi
#endif