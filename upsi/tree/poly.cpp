#include "poly.h"

namespace upsi {

void Poly::interpolation(std::vector<oc::block>& keys, std::vector<oc::block>& values) {
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

        oc::block inv = gf128Inverse(denom);
        // oc::block inv = oc::ZeroBlock;
        oc::block scale = values[i].gf128Mul(inv);
        // std::cout << keys[i] << " " << values[i] << ":\n";
        // std::cout << denom << " " << inv << std::endl;
        // std::cout << inv.gf128Mul(denom) << "\n";
        std::vector<oc::block> term;
        term.resize(n, oc::ZeroBlock);
        term[n - 1] = scale;
        // for (int k = 0; k < n; ++k) std::cout << term[k] << std::endl; std::cout << "\n";
           
        for (int j = 0, num = 0; j < n; ++j) {
            if(i == j) continue;
            ++num;
            // std::cout << j << " " << num << " " << keys[j] << "\n";
            for (int k = num; k; --k) {
                term[n - k - 1] = term[n - k - 1] ^ (term[n - k].gf128Mul(keys[j]));
            }
            // for (int k = 0; k < n; ++k) std::cout << term[k] << std::endl; std::cout << "\n";
        }
        for(int j = 0; j < n; ++j) ase[j] ^= term[j];
    }
}

oc::block Poly::eval1(Element elem) {
    oc::block rs = oc::ZeroBlock;
    for(int i = n - 1; i >= 0; --i) {
        rs = rs.gf128Mul(elem);
        rs ^= ase[i];
    }
    return rs;
}


}