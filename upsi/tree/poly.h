#ifndef Poly_H
#define Poly_H
#include "../ASE.h"

namespace upsi {

class Poly : public ASE{
    public:
        size_t n;

        Poly(size_t _n = DEFAULT_NODE_SIZE):n(_n){ase.reserve(_n);}

        oc::block gf128Inverse(oc::block a) {
            oc::block rs = oc::OneBlock;
            for (int i = 0; i < 127; ++i) {
                a = a.gf128Mul(a);
                rs = rs.gf128Mul(a);
            }
            return rs;
        }

        void interpolation(std::vector<oc::block>& keys, std::vector<oc::block>& values);

        oc::block eval1(Element elem) override;
        
};
} // namespace upsi

#endif
