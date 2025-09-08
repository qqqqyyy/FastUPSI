#include "oprf.h"

namespace upsi{

// push back oprf values into "values"
template<typename ASEType>
void OPRF<ASEType>::sender(const std::vector<Element>& input, size_t index, ASEType& b, oc::block delta,
    OPRFValueVec& values, oc::block ro_seed){
    
    // a = b + delta * okvs
    // you may use b.eval(input[i], ...), gf128Mul
    // key = input[i], value = random_oracle(key, ro_seed)
    // use random_oracle_256 for the hash outside
    // TODO


}

//push back oprf values into "values"
template<typename ASEType>
void OPRF<ASEType>::receiver(const std::vector<Element>& input, size_t index, ASEType& a, 
    OPRFValueVec& values, oc::block ro_seed){
    
    // TODO

}

template class OPRF<Poly>;
template class OPRF<rb_okvs>;

} // namespace upsi