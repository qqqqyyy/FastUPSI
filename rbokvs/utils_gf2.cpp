#include "utils_gf2.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cstdint>

namespace rbokvs {


namespace tinysha3 {

static inline uint64_t rol(uint64_t x, unsigned n){ return (x << n) | (x >> (64 - n)); }

static void keccakf(uint64_t s[25]) {
    static const uint64_t RC[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL,
        0x800000000000808aULL, 0x8000000080008000ULL,
        0x000000000000808bULL, 0x0000000080000001ULL,
        0x8000000080008081ULL, 0x8000000000008009ULL,
        0x000000000000008aULL, 0x0000000000000088ULL,
        0x0000000080008009ULL, 0x000000008000000aULL,
        0x000000008000808bULL, 0x800000000000008bULL,
        0x8000000000008089ULL, 0x8000000000008003ULL,
        0x8000000000008002ULL, 0x8000000000000080ULL,
        0x000000000000800aULL, 0x800000008000000aULL,
        0x8000000080008081ULL, 0x8000000000008080ULL,
        0x0000000080000001ULL, 0x8000000080008008ULL
    };
    static const unsigned r[24] = {
        1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14,
        27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44
    };
    for (int round = 0; round < 24; ++round) {
        uint64_t C[5], D[5];

        // theta
        for (int x = 0; x < 5; ++x)
            C[x] = s[x] ^ s[x + 5] ^ s[x + 10] ^ s[x + 15] ^ s[x + 20];
        for (int x = 0; x < 5; ++x) {
            D[x] = C[(x + 4) % 5] ^ rol(C[(x + 1) % 5], 1);
        }
        for (int x = 0; x < 5; ++x) {
            for (int y = 0; y < 5; ++y) s[x + 5*y] ^= D[x];
        }

        // rho + pi
        uint64_t cur = s[1];
        int x = 1, y = 0;
        for (int i = 0; i < 24; ++i) {
            int X = y;
            int Y = (2*x + 3*y) % 5;
            int idx = X + 5*Y;
            uint64_t tmp = s[idx];
            s[idx] = rol(cur, r[i]);
            cur = tmp;
            x = X; y = Y;
        }

        // chi
        for (int y0 = 0; y0 < 5; ++y0) {
            uint64_t row[5];
            for (int x0 = 0; x0 < 5; ++x0) row[x0] = s[x0 + 5*y0];
            for (int x0 = 0; x0 < 5; ++x0)
                s[x0 + 5*y0] = row[x0] ^ ((~row[(x0+1)%5]) & row[(x0+2)%5]);
        }

        // iota
        s[0] ^= RC[round];
    }
}

static void sha3_256_one(const uint8_t* in, size_t inlen, uint8_t out[32]){
    uint64_t st[25]{};
    const size_t rate = 136; // bytes
    // absorb full blocks
    while (inlen >= rate) {
        for (size_t i = 0; i < rate/8; ++i) {
            uint64_t w = 0;
            std::memcpy(&w, in + 8*i, 8); // little-endian
            st[i] ^= w;
        }
        keccakf(st);
        in += rate; inlen -= rate;
    }
    // last partial block with padding 0x06 ... 0x80
    uint8_t last[rate]; std::memset(last, 0, rate);
    if (inlen) std::memcpy(last, in, inlen);
    last[inlen] ^= 0x06;
    last[rate-1] ^= 0x80;
    for (size_t i = 0; i < rate/8; ++i) {
        uint64_t w = 0;
        std::memcpy(&w, last + 8*i, 8);
        st[i] ^= w;
    }
    keccakf(st);
    // squeeze 32 bytes (fits in one block)
    for (size_t i = 0; i < 4; ++i) {
        std::memcpy(out + 8*i, &st[i], 8);
    }
}

static void sha3_256_two(const uint8_t* in1, size_t n1,
                         const uint8_t* in2, size_t n2,
                         uint8_t out[32]){
    uint64_t st[25]{};
    const size_t rate = 136;
    size_t off = 0;

    auto absorb = [&](const uint8_t* p, size_t n){
        while (n) {
            size_t lane = off % rate;
            size_t chunk = (n < (rate - lane)) ? n : (rate - lane);
            // XOR chunk into the state buffer at position 'lane'
            // Build a temp block with only the 'chunk' region set; cheaper to code.
            uint8_t buf[rate]; std::memset(buf, 0, rate);
            std::memcpy(buf + lane, p, chunk);
            for (size_t i = 0; i < rate/8; ++i) {
                uint64_t w = 0; std::memcpy(&w, buf + 8*i, 8);
                st[i] ^= w;
            }
            off += chunk; p += chunk; n -= chunk;
            if ((off % rate) == 0) keccakf(st);
        }
    };

    absorb(in1, n1);
    absorb(in2, n2);

    // padding
    size_t lane = off % rate;
    uint8_t buf[rate]; std::memset(buf, 0, rate);
    buf[lane] ^= 0x06;
    buf[rate-1] ^= 0x80;
    for (size_t i = 0; i < rate/8; ++i) {
        uint64_t w = 0; std::memcpy(&w, buf + 8*i, 8);
        st[i] ^= w;
    }
    keccakf(st);

    for (size_t i = 0; i < 4; ++i) {
        std::memcpy(out + 8*i, &st[i], 8);
    }
}

} 

    static inline void sha3_256(const std::uint8_t* in1, std::size_t n1,
                                const std::uint8_t* in2, std::size_t n2,
                                std::uint8_t out[32]) {
        if (n2 == 0) {
            tinysha3::sha3_256_one(in1, n1, out);
        } else {
            tinysha3::sha3_256_two(in1, n1, in2, n2, out);
        }
    }

    std::size_t hash_to_index(const FE2& x, const std::array<std::uint8_t,16>& r1, std::size_t range) {
        auto xb = x.to_bytes_le();
        std::uint8_t h[32];
        sha3_256(xb.data(), xb.size(), r1.data(), r1.size(), h);
        std::uint64_t w = 0;
        std::memcpy(&w, h, 8);
        return range ? static_cast<std::size_t>(w % range) : 0;
    }

    U256 hash_to_band(const FE2& x, const std::array<std::uint8_t,16>& r2) {
        auto xb = x.to_bytes_le();
        std::uint8_t h[32];
        sha3_256(xb.data(), xb.size(), r2.data(), r2.size(), h);
        U256 m{};
        std::memcpy(&m[0], h + 0,  8);
        std::memcpy(&m[1], h + 8,  8);
        std::memcpy(&m[2], h + 16, 8);
        std::memcpy(&m[3], h + 24, 8);
        return m;
    }

    std::size_t hash_to_bin(const FE2& x, const std::array<std::uint8_t,16>& r3, std::size_t numBins) {
        auto xb = x.to_bytes_le();
        std::uint8_t h[32];
        sha3_256(xb.data(), xb.size(), r3.data(), r3.size(), h);
        std::uint64_t w = 0;
        std::memcpy(&w, h, 8);
        return numBins ? static_cast<std::size_t>(w % numBins) : 0;
    }

    FE2 inner_product(const U256& m, const FE2* x, std::size_t len) {
        FE2 acc = FE2::zero();
        std::size_t bits = (len < 256) ? len : 256;

        std::size_t up = (bits < 64 ? bits : 64);
        for (std::size_t i = 0; i < up; ++i) if (m[0] & MASK64[i]) acc += x[i];
        if (bits <= 64) return acc;

        up = ((bits - 64) < 64 ? (bits - 64) : 64);
        for (std::size_t i = 0; i < up; ++i) if (m[1] & MASK64[i]) acc += x[64 + i];
        if (bits <= 128) return acc;

        up = ((bits - 128) < 64 ? (bits - 128) : 64);
        for (std::size_t i = 0; i < up; ++i) if (m[2] & MASK64[i]) acc += x[128 + i];
        if (bits <= 192) return acc;

        up = (bits - 192);
        for (std::size_t i = 0; i < up; ++i) if (m[3] & MASK64[i]) acc += x[192 + i];
        return acc;
    }

    std::tuple<std::vector<U256>, std::vector<std::size_t>, std::vector<FE2>>
    create_sorted_matrix(std::size_t columns,
                         std::size_t band_width,
                         const std::array<std::uint8_t,16>& r1,
                         const std::array<std::uint8_t,16>& r2,
                         const std::vector<std::pair<FE2,FE2>>& input)
    {
        const std::size_t n = input.size();

        std::vector<std::pair<std::size_t,std::size_t>> start_pos(n); // (row_id, start)
        std::vector<U256> matrix(n);
        std::vector<std::size_t> start_ids(n);
        std::vector<FE2> y(n);

        for (std::size_t i = 0; i < n; ++i) {
            auto si = hash_to_index(input[i].first, r1, columns - band_width);
            start_pos[i] = { i, si };
        }

        std::stable_sort(start_pos.begin(), start_pos.end(),
                        [](auto a, auto b){ return a.second < b.second; });

        for (std::size_t i = 0; i < n; ++i) {
            const auto src = start_pos[i].first;
            matrix[i] = hash_to_band(input[src].first, r2);
            y[i] = input[src].second;
            start_ids[i] = start_pos[i].second;
        }

        return {std::move(matrix), std::move(start_ids), std::move(y)};
    }

    static inline void mask_to_row(const U256& m, std::vector<FE2>& row, std::size_t band_width){
        row.assign(band_width, FE2::zero());
        for (int limb = 0; limb < 4; ++limb) {
            for (int k = 0; k < 64; ++k) {
                std::size_t idx = static_cast<std::size_t>(limb)*64 + static_cast<std::size_t>(k);
                if (idx >= band_width) return;
                if (m[limb] & MASK64[k]) row[idx] = FE2::one();
            }
        }
    }

    std::vector<FE2> simple_gauss(std::vector<FE2> y,
                                  const std::vector<U256>& bands,
                                  const std::vector<std::size_t>& start_pos,
                                  std::size_t cols,
                                  std::size_t band_width)
    {
        const std::size_t rows = bands.size();
        if (rows != start_pos.size() || rows != y.size())
            throw std::invalid_argument("simple_gauss(GF2^128): mismatched dimensions");
        if (band_width == 0 || band_width > 256)
            throw std::invalid_argument("simple_gauss(GF2^128): band_width must be in 1..=256");

        std::vector<std::size_t> pivot(rows, 0);
        std::vector<std::size_t> first_nonzero(rows, band_width);

        std::vector<std::vector<FE2>> A(rows, std::vector<FE2>(band_width, FE2::zero()));
        for (std::size_t i = 0; i < rows; ++i) mask_to_row(bands[i], A[i], band_width);

        // forward elimination
        for (std::size_t i = 0; i < rows; ++i) {
            std::size_t lead = band_width;
            for (std::size_t j = 0; j < band_width; ++j) {
                if (A[i][j] != FE2::zero()) { lead = j; break; }
            }
            if (lead == band_width) throw std::runtime_error(std::string("ZeroRow ")+std::to_string(i));
            first_nonzero[i] = lead;
            pivot[i] = lead + start_pos[i];

            // normalize row i
            FE2 lead_val = A[i][lead];
            if (lead_val != FE2::one()) {
                FE2 inv = lead_val.inv();
                for (std::size_t j = lead; j < band_width; ++j) A[i][j] *= inv;
                y[i] *= inv;
            }

            // eliminate from rows below that overlap the pivot column
            for (std::size_t j = i + 1; j < rows; ++j) {
                if (start_pos[j] > pivot[i]) break;
                std::size_t off = pivot[i] - start_pos[j];
                if (off >= band_width) continue;
                if (A[j][off] != FE2::zero()) {
                    FE2 factor = A[j][off];
                    std::size_t width_i = band_width - lead;
                    std::size_t width_j = band_width - off;
                    std::size_t width   = (width_i < width_j ? width_i : width_j);
                    for (std::size_t k = 0; k < width; ++k) {
                        A[j][off + k] -= factor * A[i][lead + k];
                    }
                    y[j] -= factor * y[i];
                }
            }
        }

        // back substitution
        std::vector<FE2> x(cols, FE2::zero());
        for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(rows) - 1; i >= 0; --i) {
            FE2 sum = y[static_cast<std::size_t>(i)];
            const std::size_t si = start_pos[static_cast<std::size_t>(i)];
            for (std::size_t j = 0; j < band_width; ++j) {
                if (A[static_cast<std::size_t>(i)][j] == FE2::zero()) continue;
                FE2 xj = x[si + j];
                if (xj != FE2::zero()) sum -= A[static_cast<std::size_t>(i)][j] * xj;
            }
            x[pivot[static_cast<std::size_t>(i)]] = sum;
        }

        return x;
    }

} 
