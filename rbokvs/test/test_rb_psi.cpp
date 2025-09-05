// test_rb_psi.cpp — TCP test for rbpsi
// Modes:
//   dual <nX> <nY> <k> [seed] [port] [print]
//   recv <port> <nX> <nY> <k> [seed] [print]
//   send <host> <port> <nX> <nY> <k> [seed]
//   Eg: ./test_rb_psi dual 1000 1200 100 42 9090 print
// It makes sets X,Y with exactly k overlaps and checks the result.
// Prints timing for recv/send/total (ms).


#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "rb_psi.hpp"   

using FE = rbokvs::FE2;
using Clock = std::chrono::steady_clock;

static inline double ms_between(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

// static inline FE fe_from_u64(uint64_t v) {
//     uint8_t b[32] = {0};
//     std::memcpy(b, &v, 8);                  // little-endian
//     return FE::from_bytes_le(b, 32);
// }
static inline FE fe_from_u64(uint64_t v) {
    uint8_t b[16] = {0};
    std::memcpy(b, &v, 8);
    return FE::from_bytes_le(b, 16);
}

static inline std::string fe_hex(const FE& x) {
    auto a = x.to_bytes_le();
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (auto c : a) os << std::setw(2) << (int)c;
    return os.str();
}

// ---- build X,Y with exactly k overlaps (deterministic by seed) ----
static void make_sets(size_t nX, size_t nY, size_t k, uint64_t seed,
                      std::vector<FE>& X, std::vector<FE>& Y, std::vector<FE>& Iexp)
{
    if (k > nX || k > nY) throw std::runtime_error("k too large");
    X.clear(); Y.clear(); Iexp.clear();
    X.reserve(nX); Y.reserve(nY); Iexp.reserve(k);

    // three disjoint ranges
    uint64_t baseX = 1;
    uint64_t baseY = 2000000;
    uint64_t baseI = 1000000;

    for (size_t i = 0; i < nX - k; ++i) X.push_back(fe_from_u64(baseX + i));
    for (size_t i = 0; i < k;    ++i) { auto v = fe_from_u64(baseI + i); X.push_back(v); Iexp.push_back(v); }
    for (size_t i = 0; i < k;    ++i)  Y.push_back(fe_from_u64(baseI + i));
    for (size_t i = 0; i < nY - k; ++i) Y.push_back(fe_from_u64(baseY + i));

    std::mt19937_64 rng(seed);
    std::shuffle(X.begin(), X.end(), rng);
    std::shuffle(Y.begin(), Y.end(), rng);
}

static void print_vec(const char* name, const std::vector<FE>& V, size_t maxn=8) {
    std::cout << name << " (n=" << V.size() << "): ";
    for (size_t i=0; i<std::min(V.size(), maxn); ++i) {
        if (i) std::cout << ", ";
        std::cout << fe_hex(V[i]).substr(0, 12);
    }
    if (V.size() > maxn) std::cout << ", ...";
    std::cout << "\n";
}

static bool fe_less(const FE& a, const FE& b) { return fe_hex(a) < fe_hex(b); }

// ---- run both sides in one process (useful for CI) ----
static int run_dual(size_t nX, size_t nY, size_t k, uint64_t seed,
                    uint16_t port, bool prn)
{
    std::vector<FE> X, Y, Iexp;
    make_sets(nX, nY, k, seed, X, Y, Iexp);
    if (prn) { print_vec("X", X); print_vec("Y", Y); print_vec("I* (expected)", Iexp, 1000); }

    std::vector<FE> Igot;
    std::exception_ptr eptr = nullptr;

    double recv_ms = -1.0, send_ms = -1.0, total_ms = -1.0;
    auto t_total_s = Clock::now();

    // start receiver (server) in a thread
    std::thread th([&](){
        try {
            auto t0 = Clock::now();
            Igot = rbpsi::psi_recv(port, X);
            auto t1 = Clock::now();
            recv_ms = ms_between(t0, t1);
        } catch (...) {
            eptr = std::current_exception();
        }
    });

    // small delay to ensure listen() is ready
    std::this_thread::sleep_for(std::chrono::milliseconds(120));

    // run sender (client) in main thread
    try {
        auto t0 = Clock::now();
        rbpsi::psi_send("127.0.0.1", port, Y);
        auto t1 = Clock::now();
        send_ms = ms_between(t0, t1);
    } catch (...) {
        if (th.joinable()) th.join();
        throw;
    }
    th.join();
    if (eptr) std::rethrow_exception(eptr);
    total_ms = ms_between(t_total_s, Clock::now());

    std::sort(Igot.begin(), Igot.end(), fe_less);
    std::sort(Iexp.begin(), Iexp.end(), fe_less);

    bool ok = (Igot == Iexp);
    std::cout << "[DUAL] got |X∩Y|=" << Igot.size()
              << " (expected " << Iexp.size() << ") -> "
              << (ok ? "PASS" : "FAIL") << "\n";
    if (prn) print_vec("I (got)", Igot, 1000);

    std::cout << std::fixed << std::setprecision(3)
              << "[TIME] recv=" << recv_ms << " ms, "
              << "send=" << send_ms << " ms, "
              << "total=" << total_ms << " ms\n";
    return ok ? 0 : 2;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr <<
          "Usage:\n"
          "  dual <nX> <nY> <k> [seed] [port] [print]\n"
          "  recv <port> <nX> <nY> <k> [seed] [print]\n"
          "  send <host> <port> <nX> <nY> <k> [seed]\n";
        return 1;
    }

    std::string mode = argv[1];
    try {
        if (mode == "dual") {
            if (argc < 5) { std::cerr << "dual args missing\n"; return 1; }
            size_t nX = std::stoul(argv[2]);
            size_t nY = std::stoul(argv[3]);
            size_t k  = std::stoul(argv[4]);
            uint64_t seed = (argc >= 6 ? std::stoull(argv[5]) : 42);
            uint16_t port = (argc >= 7 ? static_cast<uint16_t>(std::stoi(argv[6])) : 9090);
            bool prn = (argc >= 8);
            return run_dual(nX, nY, k, seed, port, prn);

        } else if (mode == "recv") {
            if (argc < 6) { std::cerr << "recv args missing\n"; return 1; }
            uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
            size_t nX = std::stoul(argv[3]);
            size_t nY = std::stoul(argv[4]); // only for expected check
            size_t k  = std::stoul(argv[5]);
            uint64_t seed = (argc >= 7 ? std::stoull(argv[6]) : 42);
            bool prn = (argc >= 8);

            std::vector<FE> X, Y, Iexp;
            make_sets(nX, nY, k, seed, X, Y, Iexp);
            if (prn) print_vec("X", X);

            auto t0 = Clock::now();
            auto Igot = rbpsi::psi_recv(port, X);
            auto t1 = Clock::now();
            double recv_ms = ms_between(t0, t1);

            std::sort(Igot.begin(), Igot.end(), fe_less);
            std::sort(Iexp.begin(), Iexp.end(), fe_less);
            bool ok = (Igot == Iexp);
            std::cout << "[Receiver] got |X∩Y|=" << Igot.size()
                      << " (expected " << Iexp.size() << ") -> "
                      << (ok ? "PASS" : "FAIL") << "\n";
            if (prn) print_vec("I (recv)", Igot, 1000);

            std::cout << std::fixed << std::setprecision(3)
                      << "[TIME] recv=" << recv_ms << " ms\n";
            return ok ? 0 : 2;

        } else if (mode == "send") {
            if (argc < 7) { std::cerr << "send args missing\n"; return 1; }
            std::string host = argv[2];
            uint16_t port = static_cast<uint16_t>(std::stoi(argv[3]));
            size_t nX = std::stoul(argv[4]);   // only to mirror set-gen
            size_t nY = std::stoul(argv[5]);
            size_t k  = std::stoul(argv[6]);
            uint64_t seed = (argc >= 8 ? std::stoull(argv[7]) : 42);

            std::vector<FE> X, Y, Iexp;
            make_sets(nX, nY, k, seed, X, Y, Iexp);

            auto t0 = Clock::now();
            rbpsi::psi_send(host, port, Y);
            auto t1 = Clock::now();
            double send_ms = ms_between(t0, t1);

            std::cout << "[Sender] tags sent for |Y|=" << Y.size() << "\n";
            std::cout << std::fixed << std::setprecision(3)
                      << "[TIME] send=" << send_ms << " ms\n";
            return 0;

        } else {
            std::cerr << "unknown mode: " << mode << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERR: " << e.what() << "\n";
        return 3;
    }
}
