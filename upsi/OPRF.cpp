#include "oprf.h"

namespace upsi{

// push back oprf values into "values"
template<typename ASEType>
void OPRF<ASEType>::sender(const std::vector<Element>& input, ASEType& b, oc::block delta,
    OPRFValueVec& values, oc::block ro_seed, oc::PRNG* prng){
    
    // a = b + delta * okvs
    // you may use b.eval(input[i], ...), gf128Mul
    // key = input[i], value = random_oracle(key, ro_seed)
    // use random_oracle_256 for the hash outside
    // TODO


}

//push back oprf values into "values", return ase_diff
template<typename ASEType>
ASE OPRF<ASEType>::receiver(const std::vector<Element>& input, ASEType* ase, VoleReceiver* vole_receiver, 
    OPRFValueVec& values, oc::block ro_seed, oc::PRNG* prng){
    
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