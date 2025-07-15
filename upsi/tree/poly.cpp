#include "poly.h"

namespace upsi {

void Poly::interpolation(std::vector<oc::block>& keys, std::vector<oc::block> values) {
    if((keys.size() != n) || (values.size() != n)) {
        throw std::runtime_error("polynomial interpolation: size incorrect");
    }

    ase.resize(n, oc::ZeroBlock);

    for (int i = 0; i < n; ++i) {
        oc::block denom = oc::OneBlock;

        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            denom = denom.gf128Mul(keys[j] ^ keys[i]);
        }

        oc::block scale = values[i].gf128Mul(gf128Inverse(denom));
        std::vector<oc::block> term;
        term.resize(n, oc::ZeroBlock);
        term[n - 1] = scale;
           
        for (int j = 0, num = 0; j < n; ++j) {
            if(i == j) continue;
            ++num;
            for (int k = num; k; --k) {
                term[n - k - 1] ^= (term[n - k].gf128Mul(keys[j]));
            }
        }
        for(int j = 0; j < n; ++j) ase[j] ^= term[j];
    }
}

oc::block Poly::eval1(Element elem) {
    oc::block rs = oc::OneBlock;
    for(int i = n - 1; i >= 0; --i) {
        rs = rs.gf128Mul(elem);
        rs ^= ase[i];
    }
    return rs;
}


}