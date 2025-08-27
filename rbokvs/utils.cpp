#include "utils.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include <cryptopp/sha3.h>

namespace rbokvs {

    static inline void sha3_256(const std::uint8_t* in1, std::size_t n1,
                                const std::uint8_t* in2, std::size_t n2,
                                std::uint8_t out[32]) {
        CryptoPP::SHA3_256 hasher;
        hasher.Update(in1, n1);
        hasher.Update(in2, n2);
        hasher.Final(out);
    }

    std::size_t hash_to_index(const FE& x, const std::array<std::uint8_t,16>& r1, std::size_t range) {
        auto xb = x.to_bytes_le();   // 32 bytes LE
        std::uint8_t h[32];
        sha3_256(xb.data(), xb.size(), r1.data(), r1.size(), h);
        std::uint64_t w = 0;
        std::memcpy(&w, h, 8);       // take low 64 bits little-endian
        return range ? static_cast<std::size_t>(w % range) : 0;
    }

    U256 hash_to_band(const FE& x, const std::array<std::uint8_t,16>& r2) {
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

    std::size_t hash_to_bin(const FE& x, const std::array<std::uint8_t,16>& r3, std::size_t numBins) {
        auto xb = x.to_bytes_le();
        std::uint8_t h[32];
        sha3_256(xb.data(), xb.size(), r3.data(), r3.size(), h);
        std::uint64_t w = 0;
        std::memcpy(&w, h, 8);
        return numBins ? static_cast<std::size_t>(w % numBins) : 0;
    }


    FE inner_product(const U256& m, const FE* x, std::size_t len) {
        FE acc = FE::zero();
        std::size_t bits = (len < 256) ? len : 256;

        // limb 0
        std::size_t up = bits < 64 ? bits : 64;
        for (std::size_t i = 0; i < up; ++i) if (m[0] & MASK64[i]) acc += x[i];
        if (bits <= 64) return acc;

        // limb 1
        up = (bits - 64) < 64 ? (bits - 64) : 64;
        for (std::size_t i = 0; i < up; ++i) if (m[1] & MASK64[i]) acc += x[64 + i];
        if (bits <= 128) return acc;

        // limb 2
        up = (bits - 128) < 64 ? (bits - 128) : 64;
        for (std::size_t i = 0; i < up; ++i) if (m[2] & MASK64[i]) acc += x[128 + i];
        if (bits <= 192) return acc;

        // limb 3
        up = bits - 192;
        for (std::size_t i = 0; i < up; ++i) if (m[3] & MASK64[i]) acc += x[192 + i];

        return acc;
    }

    std::tuple<std::vector<U256>, std::vector<std::size_t>, std::vector<FE>>
    create_sorted_matrix(std::size_t columns,
                        std::size_t band_width,
                        const std::array<std::uint8_t,16>& r1,
                        const std::array<std::uint8_t,16>& r2,
                        const std::vector<std::pair<FE,FE>>& input){
        const std::size_t n = input.size();

        std::vector<std::pair<std::size_t,std::size_t>> start_pos(n); // (row_id, start)
        std::vector<U256> matrix(n);
        std::vector<std::size_t> start_ids(n);
        std::vector<FE> y(n);

        // compute band starts
        for (std::size_t i = 0; i < n; ++i) {
            auto si = hash_to_index(input[i].first, r1, columns - band_width);
            start_pos[i] = { i, si };
        }

        // sort rows by start column (stable)
        std::stable_sort(start_pos.begin(), start_pos.end(),
                        [](auto a, auto b){ return a.second < b.second; });

        // build matrix rows (band masks) and RHS in sorted order
        for (std::size_t i = 0; i < n; ++i) {
            const auto src = start_pos[i].first;
            matrix[i] = hash_to_band(input[src].first, r2);
            y[i] = input[src].second;
            start_ids[i] = start_pos[i].second;
        }

        return {std::move(matrix), std::move(start_ids), std::move(y)};
    }

    static inline void mask_to_row(const U256& m, std::vector<FE>& row, std::size_t band_width){
        row.assign(band_width, FE::zero());
        
        // up to 4 limbs * 64 bits, stop at band_width
        for (int limb = 0; limb < 4; ++limb) {
            for (int k = 0; k < 64; ++k) {
                std::size_t idx = static_cast<std::size_t>(limb)*64 + static_cast<std::size_t>(k);
                if (idx >= band_width) return;
                if (m[limb] & MASK64[k]) row[idx] = FE::one();
            }
        }
    }

    std::vector<FE> simple_gauss(std::vector<FE> y,
                                const std::vector<U256>& bands,
                                const std::vector<std::size_t>& start_pos,
                                std::size_t cols,
                                std::size_t band_width)
    {
        const std::size_t rows = bands.size();
        if (rows != start_pos.size() || rows != y.size())
            throw std::invalid_argument("simple_gauss: mismatched dimensions");
        if (band_width == 0 || band_width > 256)
            throw std::invalid_argument("simple_gauss: band_width must be in 1..=256");

        std::vector<std::size_t> pivot(rows, 0);
        std::vector<std::size_t> first_nonzero(rows, band_width);

        // materialize band rows as FE
        std::vector<std::vector<FE>> A(rows, std::vector<FE>(band_width, FE::zero()));
        for (std::size_t i = 0; i < rows; ++i)
            mask_to_row(bands[i], A[i], band_width);

        // forward elimination (within band overlaps)
        for (std::size_t i = 0; i < rows; ++i) {
            // find first nonzero in row i
            std::size_t lead = band_width;
            for (std::size_t j = 0; j < band_width; ++j) {
                if (A[i][j] != FE::zero()) { lead = j; break; }
            }
            if (lead == band_width) {
                throw std::runtime_error(std::string("ZeroRow ") + std::to_string(i));
            }
            first_nonzero[i] = lead;
            pivot[i] = lead + start_pos[i];

            // count how many following rows overlap this pivot column
            std::size_t cnt = 0;
            for (std::size_t j = i + 1; j < rows; ++j) {
                if (start_pos[j] > pivot[i]) break;
                ++cnt;
            }
            if (cnt == 0) {
                // skip normalization + elimination now; will normalize later
                continue;
            }

            // normalize row i to leading 1
            FE inv = A[i][lead].inv();
            for (std::size_t j = lead; j < band_width; ++j) A[i][j] *= inv;
            y[i] *= inv;

            // eliminate from rows below within overlapping window
            for (std::size_t j = i + 1; j < rows; ++j) {
                if (start_pos[j] > pivot[i]) break;
                const std::size_t off = pivot[i] - start_pos[j]; // local index into row j
                if (A[j][off] != FE::zero()) {
                    FE factor = A[j][off];
                    for (std::size_t k = 0; k + lead < band_width; ++k) {
                        A[j][k + off] -= factor * A[i][k + lead];
                    }
                    y[j] -= factor * y[i];
                }
            }
        }

        // ensure leading coefficient = 1 for all rows (like the Rust cleanup pass)
        for (std::size_t i = 0; i < rows; ++i) {
            FE lead = A[i][first_nonzero[i]];
            if (lead != FE::one()) {
                FE inv = lead.inv();
                for (std::size_t j = first_nonzero[i]; j < band_width; ++j) A[i][j] *= inv;
                y[i] *= inv;
            }
        }

        // back substitution: produce x with size = cols
        std::vector<FE> x(cols, FE::zero());
        for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(rows) - 1; i >= 0; --i) {
            FE sum = y[static_cast<std::size_t>(i)];
            const std::size_t si = start_pos[static_cast<std::size_t>(i)];
            // subtract known contributions inside the band
            for (std::size_t j = 0; j < band_width; ++j) {
                if (A[static_cast<std::size_t>(i)][j] == FE::zero()) continue;
                FE xj = x[si + j];
                if (xj != FE::zero()) sum -= A[static_cast<std::size_t>(i)][j] * xj;
            }
            x[pivot[static_cast<std::size_t>(i)]] = sum;
        }

        return x;
    }

}
