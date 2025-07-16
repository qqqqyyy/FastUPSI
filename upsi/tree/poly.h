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

        int computeDeg(oc::block& a) {
            uint64_t hi, lo;
            std::tie(lo, hi) = a.get<uint64_t>();
            // std::cout << std::hex << hi << " " << lo << "\n";
            if (hi != 0) {
                return 64 + (63 - __builtin_clzll(hi));  // high bits
            } else if (lo != 0) {
                return 63 - __builtin_clzll(lo);         // low bits
            } else {
                return -1;
            }
        }

        inline oc::block block_shift_left(const oc::block& a, int shift) {
            if(shift == 0) return a;
            uint64_t hi, lo;
            std::tie(lo, hi) = a.get<uint64_t>();

            uint64_t new_hi, new_lo;
            if (shift >= 64) {
                new_hi = lo << (shift - 64);
                new_lo = 0;
            } else {
                new_hi = (hi << shift) | (lo >> (64 - shift));
                new_lo = lo << shift;
            }

            return oc::toBlock(new_hi, new_lo);
        }

        void exgcd(oc::block a, oc::block b, oc::block& x, oc::block &y) {
            // std::cout << a << " " << b << std::endl;
            if(b == oc::ZeroBlock) {
                x = oc::OneBlock; y = oc::ZeroBlock;
                return;
            }
            int degA = computeDeg(a), degB = computeDeg(b);
            int shift = std::max(0, degA - degB);
            exgcd(b, a ^ (block_shift_left(b, shift)), y, x);
            // y ^= block_shift_left(x, shift);
            if(shift < 64) y ^= x.gf128Mul(oc::toBlock(1ull << shift));
            else y ^= x.gf128Mul(oc::toBlock(1ull << (shift - 64), 0));
        }

        oc::block gf128Inverse_EEA(oc::block a) {
            oc::block x, y;
            oc::block b = oc::toBlock(0, 0x87);
            int degA = computeDeg(a), shift = 128 - degA;
            // std::cout << a << " " << degA << " " << "\n";

            exgcd(a, b ^ block_shift_left(a, shift), x, y);
            // x ^= block_shift_left(y, 128 - degA);
            if(shift < 64) x ^= y.gf128Mul(oc::toBlock(1ull << shift));
            else x ^= y.gf128Mul(oc::toBlock(1ull << (shift-64), 0));

            return x;
        }

        void interpolation(std::vector<oc::block>& keys, std::vector<oc::block>& values);

        oc::block eval1(Element elem) override;
        
};
} // namespace upsi

#endif
