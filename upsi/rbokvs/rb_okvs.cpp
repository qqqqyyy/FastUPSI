#include "rb_okvs.h"

#include "rb_okvs.h"
#include <algorithm>
#include <tuple>
#include <stdexcept>

namespace upsi {

    using U256 = std::array<oc::u64, 4>;

    static constexpr oc::u64 MASK64[64] = {
        1ull<<0,1ull<<1,1ull<<2,1ull<<3,1ull<<4,1ull<<5,1ull<<6,1ull<<7,
        1ull<<8,1ull<<9,1ull<<10,1ull<<11,1ull<<12,1ull<<13,1ull<<14,1ull<<15,
        1ull<<16,1ull<<17,1ull<<18,1ull<<19,1ull<<20,1ull<<21,1ull<<22,1ull<<23,
        1ull<<24,1ull<<25,1ull<<26,1ull<<27,1ull<<28,1ull<<29,1ull<<30,1ull<<31,
        1ull<<32,1ull<<33,1ull<<34,1ull<<35,1ull<<36,1ull<<37,1ull<<38,1ull<<39,
        1ull<<40,1ull<<41,1ull<<42,1ull<<43,1ull<<44,1ull<<45,1ull<<46,1ull<<47,
        1ull<<48,1ull<<49,1ull<<50,1ull<<51,1ull<<52,1ull<<53,1ull<<54,1ull<<55,
        1ull<<56,1ull<<57,1ull<<58,1ull<<59,1ull<<60,1ull<<61,1ull<<62,1ull<<63
    };

    static inline U256 split256(const std::pair<oc::block, oc::block>& H) {
        U256 m{};
        auto lo = H.first.get<oc::u64>();
        auto hi = H.second.get<oc::u64>();
        m[0] = lo[0]; m[1] = lo[1];
        m[2] = hi[0]; m[3] = hi[1];
        return m;
    }

    static inline oc::u64 fold64(oc::block b) {
        return b.get<oc::u64>()[0] ^ b.get<oc::u64>()[1];
    }

    // hash -> start index in [0, range)
    static inline std::size_t hash_to_index(Element x, oc::block r1, std::size_t range) {
        auto H = random_oracle_256(x, /*index=*/0, r1);
        oc::u64 w = fold64(H.first) ^ fold64(H.second);
        return range ? static_cast<std::size_t>(w % range) : 0;
    }

    // hash -> 256-bit local band mask
    static inline U256 hash_to_band(Element x, oc::block r2) {
        auto H = random_oracle_256(x, /*index=*/1, r2);
        return split256(H);
    }

    static inline void mask_to_row(const U256& m, std::vector<uint8_t>& row, std::size_t band_width){
        row.assign(band_width, 0);
        for (int limb = 0; limb < 4; ++limb) {
            for (int k = 0; k < 64; ++k) {
                std::size_t idx = static_cast<std::size_t>(limb)*64 + static_cast<std::size_t>(k);
                if (idx >= band_width) return;
                if (m[limb] & MASK64[k]) row[idx] = 1;
            }
        }
    }

    static inline
    std::tuple<std::vector<U256>, std::vector<std::size_t>, std::vector<Element>>
    create_sorted_matrix(std::size_t columns,
                        std::size_t band_width,
                        oc::block r1, oc::block r2,
                        const std::vector<std::pair<Element,Element>>& input)
    {
        const std::size_t rows = input.size();

        std::vector<std::pair<std::size_t,std::size_t>> start_pos(rows); // (row id, start col)
        std::vector<U256> bands(rows);
        std::vector<std::size_t> starts(rows);
        std::vector<Element> rhs(rows);

        // place each row so its band fits within [0,columns)
        for (std::size_t i = 0; i < rows; ++i) {
            auto s = hash_to_index(input[i].first, r1, columns - band_width);
            start_pos[i] = { i, s };
        }

        std::stable_sort(start_pos.begin(), start_pos.end(),
                        [](auto a, auto b){ return a.second < b.second; });

        for (std::size_t i = 0; i < rows; ++i) {
            const auto src = start_pos[i].first;
            bands[i]  = hash_to_band(input[src].first, r2);
            rhs[i]    = input[src].second;
            starts[i] = start_pos[i].second;
        }
        return { std::move(bands), std::move(starts), std::move(rhs) };
    }

    // Solve A * X = Y over GF(2) with banded rows (0/1), X/Y are oc::block
    static inline std::vector<Element>
    simple_gauss_xor(std::vector<Element> y,
                    const std::vector<U256>& bands,
                    const std::vector<std::size_t>& start_pos,
                    std::size_t columns,
                    std::size_t band_width)
    {
        const std::size_t rows = bands.size();
        if (rows != start_pos.size() || rows != y.size())
            throw std::invalid_argument("simple_gauss_xor: dimension mismatch");
        if (band_width == 0 || band_width > 256)
            throw std::invalid_argument("simple_gauss_xor: bad band width");

        std::vector<std::vector<uint8_t>> A(rows);
        for (std::size_t i = 0; i < rows; ++i)
            mask_to_row(bands[i], A[i], band_width);

        std::vector<std::size_t> pivot(rows, 0);

        // forward elimination
        for (std::size_t i = 0; i < rows; ++i) {
            // find leading 1 (local)
            std::size_t lead = band_width;
            for (std::size_t j = 0; j < band_width; ++j) {
                if (A[i][j]) { lead = j; break; }
            }
            if (lead == band_width)
                throw std::runtime_error("ZeroRow in simple_gauss_xor");
            pivot[i] = start_pos[i] + lead;

            // eliminate from subsequent overlapping rows
            for (std::size_t j = i + 1; j < rows; ++j) {
                if (start_pos[j] > pivot[i]) break; // no more overlap
                const std::size_t off = pivot[i] - start_pos[j]; // where pivot sits in row j
                if (off < band_width && A[j][off]) {
                    for (std::size_t k = 0; k + lead < band_width; ++k)
                        A[j][k + off] ^= A[i][k + lead];
                    y[j] ^= y[i];
                }
            }
        }

        // back substitution
        std::vector<Element> x(columns, oc::ZeroBlock);
        for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(rows) - 1; i >= 0; --i) {
            Element sum = y[static_cast<std::size_t>(i)];
            const std::size_t si = start_pos[static_cast<std::size_t>(i)];
            const std::size_t piv = pivot[static_cast<std::size_t>(i)];
            for (std::size_t j = 0; j < band_width; ++j) {
                if (!A[static_cast<std::size_t>(i)][j]) continue;
                const std::size_t col = si + j;
                if (col == piv) continue;
                sum ^= x[col];
            }
            x[piv] = sum;
        }
        return x;
    }

    // ================================================================
    // rb_okvs methods
    // ================================================================



    void rb_okvs::build(const std::vector<Element>& elems, oc::block ro_seed) {
        // choose per-instance seeds (persist for eval)
        // derive deterministically from ro_seed (or use PRNG seeded with ro_seed)
        setup(ro_seed);

        const std::size_t columns = n;
        if (columns < band_width_)
            throw std::runtime_error("rb_okvs::build: n < band_width");

        // prepare (key, rhs) pairs; rhs here is RO(key, ro_seed)
        std::vector<std::pair<Element,Element>> input;
        input.reserve(elems.size());
        for (auto& e : elems) {
            input.emplace_back(e, random_oracle(e, ro_seed));
        }

        // build banded system
        auto [bands, starts, rhs] =
            create_sorted_matrix(columns, band_width_, r1_, r2_, input);

        // solve for table values X
        auto X = simple_gauss_xor(std::move(rhs), bands, starts, columns, band_width_);

        // write into ASE storage
        // if (ase.size() != columns) ase.resize(columns);
        if (ase.size() != columns) throw std::runtime_error("rb_okvs ase size error");
        for (std::size_t i = 0; i < columns; ++i) {
            // if (!ase[i]) ase[i] = std::make_shared<oc::block>(oc::ZeroBlock);
            // if (!ase[i]) throw std::runtime_error("rb_okvs ase null pointer");
            // *(ase[i]) = X[i];
            ase[i] = X[i];
        }
        elem_cnt = elems.size();
    }
    
    oc::block rb_okvs::eval1(Element elem) {
        if (n == 0) return oc::ZeroBlock;
        if (band_width_ == 0) band_width_ = default_band_width(n);

        // recompute band placement (must match build)
        std::size_t start = hash_to_index(elem, r1_, n - band_width_);
        U256 mask        = hash_to_band(elem, r2_);

        std::vector<uint8_t> row;
        mask_to_row(mask, row, band_width_);

        oc::block acc = oc::ZeroBlock;
        for (std::size_t j = 0; j < band_width_; ++j) {
            if (!row[j]) continue;
            std::size_t col = start + j;
            // acc ^= *(ase[col]);
            acc ^= ase[col];
        }
        return acc;
    }


}