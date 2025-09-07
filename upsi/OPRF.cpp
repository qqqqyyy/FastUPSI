#include "oprf.h"

namespace upsi{

// push back oprf values into "values"
template<typename ASEType>
void OPRF<ASEType>::sender(const std::vector<Element>& input, ASE* ase_diff, VoleSender* vole_sender, 
    BlockVec& values, oc::block ro_seed, oc::PRNG* prng){
    
    int ase_size = ase_diff->n;
    oc::block delta = vole_sender->delta;
    ASEType b = ASEType(std::move(vole_sender->get(ase_size) + (*ase_diff * delta))); //convert base ASE type to ASEType
    
    // TODO, a = b + delta * c
    // you may use b.eval(input[i], ...), gf128Mul


}

//push back oprf values into "values", return ase_diff
template<typename ASEType>
ASE OPRF<ASEType>::receiver(const std::vector<Element>& input, ASEType* ase, VoleReceiver* vole_receiver, 
    BlockVec& values, oc::block ro_seed, oc::PRNG* prng){
    
    ase->build(input, ro_seed, prng);
    int ase_size = ase->n;
    auto vole = vole_receiver->get(ase_size);
    ASEType a = ASEType(std::move(vole.first));
    ASE c = vole.second;

    // TODO

    return *ase - c;
}

template class OPRF<Poly>;
template class OPRF<rb_okvs>;

} // namespace upsi