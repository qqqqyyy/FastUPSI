#include <random>
#include <iostream>

#include "rb_okvs.hpp"

int main() {
    using P = std::pair<rbokvs::FE, rbokvs::FE>;

    const std::size_t n = 4096;
    std::array<uint8_t,16> r1{}, r2{};
    for (int i=0;i<16;++i){
        r1[i]=uint8_t(i); r2[i]=uint8_t(32+i); 
    }

    std::mt19937_64 rng(42);
    std::vector<P> kvs; kvs.reserve(n);
    for (std::size_t i=0;i<n;++i)
        kvs.emplace_back(rbokvs::FE::from_u64(rng()), rbokvs::FE::from_u64(rng()));

    rbokvs::RbOkvs okvs(n, r1, r2);
    auto enc = okvs.encode(kvs);

    std::vector<rbokvs::FE> keys; keys.reserve(n);
    for (auto& kv : kvs) keys.push_back(kv.first);

    auto dec = okvs.decode(enc, keys);

    for (std::size_t i=0;i<n;++i)
        if (dec[i] != kvs[i].second) { std::cerr << "Mismatch @"<<i<<"\n"; return 1; }

    std::cout << "RbOkvs test OK\n";
    return 0;
}
