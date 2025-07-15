#ifndef Poly_H
#define Poly_H
#include "../ASE.h"

namespace upsi {

class Poly : public ASE{
    public:
        size_t n;

        Poly(size_t _n):n(_n){ase.reserve(_n);}

        inline oc::block gf128Inverse(oc::block a) {
            uint64_t i = -1; //2^64 - 1
            a = a.gf128Mul(a); //a^2
            a = a.gf128Pow(i); //(a^2)^(2^64-1) = a^{2^128 - 2}
            return a;
        }

        void interpolation(std::vector<oc::block>& keys, std::vector<oc::block> values);

        oc::block eval1(Element elem) override;
        
};
} // namespace upsi

#endif
