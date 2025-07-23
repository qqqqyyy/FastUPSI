#ifndef Poly_H
#define Poly_H
#include "crypto_node.h"

namespace upsi {

class Poly : public CryptoNode{
    public:
        size_t n;

        Poly(size_t _n = DEFAULT_NODE_SIZE):n(_n){
            ase.reserve(_n);
            for (int i = 0; i < _n; ++i) 
                ase.push_back(std::make_shared<oc::block>(oc::ZeroBlock));
        }
        
        void clear() override {for (int i = 0; i < n; ++i) *(ase[i]) = oc::ZeroBlock;}

        void computeDenom(const BlockVec& keys, const BlockVec& values, BlockVec& denoms);
        void interpolation(const BlockVec& keys, const BlockVec& values, BlockVec& denoms_inv, int& idx);

        oc::block eval1(Element elem) override;
        
};

void batchInterpolation(std::vector<Poly>& polys, const std::vector<BlockVec>& keys, const std::vector<BlockVec>& values);

oc::block gf128Inverse_WD(oc::block a);
oc::block gf128Inverse(oc::block a);
inline int computeDeg(oc::block& a);
inline oc::block block_shift_left(const oc::block& a, int shift);
void exgcd(oc::block a, oc::block b, oc::block& x, oc::block &y);
oc::block gf128Inverse_EEA(oc::block a);

} // namespace upsi

#endif
