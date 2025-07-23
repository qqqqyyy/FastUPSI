#include "poly.h"

namespace upsi {


/******************************************************************
******************************* POLY ******************************
******************************************************************/

void Poly::computeDenom(const BlockVec& keys, const BlockVec& values, BlockVec& denoms) {
    if((keys.size() != n) || (values.size() != n)) {
        throw std::runtime_error("polynomial interpolation: size incorrect");
    }

    for (int i = 0; i < n; ++i) {
        oc::block cur_denom = oc::OneBlock;

        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            cur_denom = cur_denom.gf128Mul(keys[j] ^ keys[i]);
        }

        denoms.push_back(cur_denom);
    }
}

void Poly::interpolation(const BlockVec& keys, const BlockVec& values, BlockVec& denoms_inv, int& idx) {
    if((keys.size() != n) || (values.size() != n)) {
        throw std::runtime_error("polynomial interpolation: size incorrect");
    }

    ase.resize(n, oc::ZeroBlock);

    for (int i = 0; i < n; ++i) {
        oc::block inv = denoms_inv[idx++];
        oc::block scale = values[i].gf128Mul(inv);
        std::vector<oc::block> term;
        term.resize(n, oc::ZeroBlock);
        term[n - 1] = scale;
           
        for (int j = 0, num = 0; j < n; ++j) {
            if(i == j) continue;
            ++num;
            for (int k = num; k; --k) {
                term[n - k - 1] = term[n - k - 1] ^ (term[n - k].gf128Mul(keys[j]));
            }
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

void batchInterpolation(std::vector<Poly>& polys, const std::vector<BlockVec>& keys, const std::vector<BlockVec>& values){
    int cnt = polys.size();

    BlockVec denoms, denoms_inv, tmp;
    for (int i = 0; i < cnt; ++i) {
        polys[i].computeDenom(keys[i], values[i], denoms);
    }
    
    int denoms_cnt = denoms.size();
    denoms_inv.resize(denoms_cnt); 
    tmp.resize(denoms_cnt + 1);
    tmp[0] = oc::OneBlock;
    for (int i = 0; i < denoms_cnt; ++i) tmp[i + 1] = tmp[i].gf128Mul(denoms[i]);
    tmp[denoms_cnt] = gf128Inverse_EEA(tmp[denoms_cnt]);
    for (int i = denoms_cnt - 1; i >= 0; --i) {
        denoms_inv[i] = tmp[i + 1].gf128Mul(tmp[i]);
        tmp[i] = tmp[i + 1].gf128Mul(denoms[i]);
    }
    
    int idx = 0;
    for (int i = 0; i < cnt; ++i) {
        polys[i].interpolation(keys[i], values[i], denoms_inv, idx);
    }
}


/******************************************************************
************************* HELPER FUNCTIONS ************************
******************************************************************/

        oc::block gf128Inverse_WD(oc::block a) {//windowing
            int WINDOW_SIZE = 2;
            oc::block last_wd = a, wd = a.gf128Mul(a);
            oc::block rs = wd;
            while(WINDOW_SIZE <= 64) {
                wd = wd.gf128Mul(last_wd);
                last_wd = wd;
                for (int i = 0; i < WINDOW_SIZE; ++i) wd = wd.gf128Mul(wd);
                rs = rs.gf128Mul(wd);
                WINDOW_SIZE <<= 1;
            }
            return rs;
        }

        oc::block gf128Inverse(oc::block a) {
            oc::block rs = oc::OneBlock;
            for (int i = 0; i < 127; ++i) {
                a = a.gf128Mul(a);
                rs = rs.gf128Mul(a);
            }
            return rs;
        }

        inline int computeDeg(oc::block& a) {
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
            else if(a == oc::ZeroBlock) {
               y = oc::OneBlock; x = oc::ZeroBlock;
               return;
            }
            int degA = computeDeg(a), degB = computeDeg(b);
            if(degA < degB) {
                int shift = degB - degA;
                exgcd(a, b ^ (block_shift_left(a, shift)), x, y);
                if(shift < 64) x ^= y.gf128Mul(oc::toBlock(1ull << shift));
                else x ^= y.gf128Mul(oc::toBlock(1ull << (shift - 64), 0));
            }
            else {
                int shift = degA - degB;
                exgcd(a ^ (block_shift_left(b, shift)), b, x, y);
                if(shift < 64) y ^= x.gf128Mul(oc::toBlock(1ull << shift));
                else y ^= x.gf128Mul(oc::toBlock(1ull << (shift - 64), 0));
            }
        }

        oc::block gf128Inverse_EEA(oc::block a) {
            oc::block x, y;
            oc::block b = oc::toBlock(0, 0x87);
            int degA = computeDeg(a), shift = 128 - degA;

            exgcd(a, b ^ block_shift_left(a, shift), x, y);
            if(shift < 64) x ^= y.gf128Mul(oc::toBlock(1ull << shift));
            else x ^= y.gf128Mul(oc::toBlock(1ull << (shift - 64), 0));

            return x;
        }


}