#include "oprf.h"

namespace upsi{

// push back oprf values into "values"
template<typename ASEType>
void OPRF<ASEType>::sender(const std::vector<Element>& input, size_t index, ASEType& b, oc::block delta,
    OPRFValueVec& values, oc::block ro_seed){
    
    // a = b + delta * okvs
    // values[i] = random_oracle_256(b.eval(input[i])+delta * random_oracle(input[i], ro_seed), ro_seed)

    // values.reserve(values.size() + input.size());
    for (const auto& x : input) {
        auto bx   = b.eval1(x);                 
        auto kx   = random_oracle(x, ro_seed);        
        auto prod = kx.gf128Mul(delta);             
        auto y    = bx ^ prod;                        // b(x) + delta*H(x)
        values.push_back(random_oracle_256(y, index, ro_seed));
    }
}

template<typename ASEType>
OPRFValue OPRF<ASEType>::sender(const Element& x, size_t index, ASEType& b, oc::block delta, oc::block ro_seed){
    
    // a = b + delta * okvs
    // values[i] = random_oracle_256(b.eval(input[i])+delta * random_oracle(input[i], ro_seed), ro_seed)

    auto bx   = b.eval1(x);                 
    auto kx   = random_oracle(x, ro_seed);        
    auto prod = kx.gf128Mul(delta);             
    auto y    = bx ^ prod;                        // b(x) + delta*H(x)
    return random_oracle_256(y, index, ro_seed);
}

//push back oprf values into "values"
template<typename ASEType>
void OPRF<ASEType>::receiver(const std::vector<Element>& input, size_t index, ASEType& a, 
    OPRFValueVec& values, oc::block ro_seed){

    // values[i] = random_oracle_256(a.eval(input[i]), ro_seed)
    // values.reserve(values.size() + input.size());
    for (const auto& x : input) {
        auto ax = a.eval1(x);                   
        values.push_back(random_oracle_256(ax, index, ro_seed)); // a(x)
    }

}

template<typename ASEType>
OPRFValue OPRF<ASEType>::receiver(const Element& x, size_t index, ASEType& a, oc::block ro_seed) {
    auto ax = a.eval1(x);
    return random_oracle_256(ax, index, ro_seed); // a(x)
}

template class OPRF<Poly>;
template class OPRF<rb_okvs>;

} // namespace upsi
