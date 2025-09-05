// ./rbokvs/test_rbokvs_gf2 --help


#include <iostream>
#include <vector>
#include <random>
#include <cstring>
#include <chrono>
#include <fstream>

#include "rb_okvs_gf2.hpp"
#include "fe2_128.hpp"

using namespace rbokvs;

static std::string fe2_hex(const FE2& x){
    auto le = x.to_bytes_le(); // 16 bytes, little-endian (lo first)
    static const char* hex = "0123456789abcdef";
    std::string s; s.reserve(32);
    for(int i=15;i>=0;--i){
        unsigned v = le[static_cast<std::size_t>(i)];
        s.push_back(hex[(v>>4)&0xF]);
        s.push_back(hex[v&0xF]);
    }
    return s;
}

int main(int argc, char** argv){
    std::size_t n = 1000;
    std::uint64_t seed = 0xC0FFEEu;
    bool have_n = false, have_seed = false;
    std::size_t printN = 0;         // 0 = don't print
    std::string dumpPath;           // non-empty -> dump all to file

    for (int i=1;i<argc;++i){
        std::string ar = argv[i];
        if (ar == "--help" || ar == "-h"){
            std::cerr << "Usage: " << argv[0] << " [n=100] [seed=0xC0FFEE] [--print-enc N] [--dump-enc path]\n";
            return 0;
        } else if (ar == "--print-enc" && i+1 < argc){
            printN = static_cast<std::size_t>(std::stoull(argv[++i]));
        } else if (ar.rfind("--dump-enc",0) == 0){
            if (ar == "--dump-enc" && i+1 < argc) dumpPath = argv[++i];
            else if (ar.rfind("--dump-enc=",0)==0) dumpPath = ar.substr(std::string("--dump-enc=").size());
        } else if (!ar.empty() && (ar.rfind("0x",0)==0 || std::isdigit(static_cast<unsigned char>(ar[0])))) {
            if (!have_n) {
                n = static_cast<std::size_t>(std::stoull(ar, nullptr, 0));
                have_n = true;
            } else if (!have_seed) {
                seed = std::stoull(ar, nullptr, 0);
                have_seed = true;
            }
        }
    }

    std::mt19937_64 gen(seed);
    auto rand64 = [&](){ return static_cast<std::uint64_t>(gen()); };

    std::array<uint8_t,16> r1{}, r2{};
    std::uint64_t a = rand64(), b = rand64(), c = rand64(), d = rand64();
    std::memcpy(r1.data()+0, &a, 8);
    std::memcpy(r1.data()+8, &b, 8);
    std::memcpy(r2.data()+0, &c, 8);
    std::memcpy(r2.data()+8, &d, 8);

    std::cout << "[Test] RB-OKVS over GF(2^128)\n";
    std::cout << "  n=" << n << " seed=" << seed << "\n";

    std::vector<std::pair<FE2,FE2>> kv;
    kv.reserve(n);
    std::uint64_t base_lo = rand64(), base_hi = rand64();
    for(std::size_t i=0;i<n;++i){
        FE2 k(base_lo + static_cast<std::uint64_t>(i), base_hi);
        FE2 v(rand64(), rand64());
        kv.emplace_back(k, v);
    }

    RbOkvsGF2 okvs(n, r1, r2);
    std::cout << "  columns=" << okvs.columns << " bandWidth=" << okvs.bandWidth << "\n";

    auto t0 = std::chrono::high_resolution_clock::now();
    auto s  = okvs.encode(kv);
    auto t1 = std::chrono::high_resolution_clock::now();

    // Optional: show some encode results
    if (printN > 0) {
        std::size_t M = std::min<std::size_t>(printN, s.size());
        std::cout << "  first " << M << " encoded slots (hex):\n";
        for (std::size_t i=0;i<M;++i){
            std::cout << "    s[" << i << "] = " << fe2_hex(s[i]) << "\n";
        }
    }
    if (!dumpPath.empty()) {
        std::ofstream ofs(dumpPath, std::ios::binary);
        if (!ofs) {
            std::cerr << "[warn] cannot open dump path: " << dumpPath << "\n";
        } else {
            ofs << "# index, hex(FE2)\n";
            for (std::size_t i=0;i<s.size();++i){
                ofs << i << "," << fe2_hex(s[i]) << "\n";
            }
            std::cout << "  encoding dumped to " << dumpPath << " (" << s.size() << " lines)\n";
        }
    }

    // now do a quick decode+verify
    std::vector<FE2> keys; keys.reserve(n);
    for(auto& p : kv) keys.push_back(p.first);
    auto vals = okvs.decode(s, keys);

    std::size_t bad = 0;
    for(std::size_t i=0;i<n;++i){
        if(vals[i] != kv[i].second){
            if(bad < 5){
                std::cerr << "  MISMATCH i=" << i
                          << " key=" << fe2_hex(kv[i].first)
                          << " got=" << fe2_hex(vals[i])
                          << " exp=" << fe2_hex(kv[i].second) << "\n";
            }
            ++bad;
        }
    }

    auto enc_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "  encode: " << enc_ms << " ms\n";

    if(bad==0){
        std::cout << "[OK] all " << n << " values verified.\n";
        return 0;
    }else{
        std::cerr << "[FAIL] mismatches=" << bad << "\n";
        return 1;
    }
}
