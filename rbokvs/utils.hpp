#pragma once
#include "types.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace rbokvs {

    std::size_t hash_to_index(const FE& x, const std::array<std::uint8_t,16>& r1, std::size_t range);
    U256        hash_to_band (const FE& x, const std::array<std::uint8_t,16>& r2);
    std::size_t hash_to_bin  (const FE& x, const std::array<std::uint8_t,16>& r3, std::size_t numBins);

    inline constexpr std::uint64_t MASK64[64] = {
        0x1ULL,0x2ULL,0x4ULL,0x8ULL,0x10ULL,0x20ULL,0x40ULL,0x80ULL,
        0x100ULL,0x200ULL,0x400ULL,0x800ULL,0x1000ULL,0x2000ULL,0x4000ULL,0x8000ULL,
        0x10000ULL,0x20000ULL,0x40000ULL,0x80000ULL,0x100000ULL,0x200000ULL,0x400000ULL,0x800000ULL,
        0x1000000ULL,0x2000000ULL,0x4000000ULL,0x8000000ULL,0x10000000ULL,0x20000000ULL,0x40000000ULL,0x80000000ULL,
        0x100000000ULL,0x200000000ULL,0x400000000ULL,0x800000000ULL,0x1000000000ULL,0x2000000000ULL,0x4000000000ULL,0x8000000000ULL,
        0x10000000000ULL,0x20000000000ULL,0x40000000000ULL,0x80000000000ULL,0x100000000000ULL,0x200000000000ULL,0x400000000000ULL,0x800000000000ULL,
        0x1000000000000ULL,0x2000000000000ULL,0x4000000000000ULL,0x8000000000000ULL,0x10000000000000ULL,0x20000000000000ULL,0x40000000000000ULL,0x80000000000000ULL,
        0x100000000000000ULL,0x200000000000000ULL,0x400000000000000ULL,0x800000000000000ULL,0x1000000000000000ULL,0x2000000000000000ULL,0x4000000000000000ULL,0x8000000000000000ULL
    };

    // Sum x[i] for each set bit i in m; x points to a window of length len (band_width)
    FE inner_product(const U256& m, const FE* x, std::size_t len);

    std::tuple<std::vector<U256>, std::vector<std::size_t>, std::vector<FE>>
    create_sorted_matrix(std::size_t columns,
                        std::size_t band_width,
                        const std::array<std::uint8_t,16>& r1,
                        const std::array<std::uint8_t,16>& r2,
                        const std::vector<std::pair<FE,FE>>& input);

    // ======================= Banded Gaussian elimination =========================
    // Solves for x in M*u = y, where rows are banded with width band_width.
    // - bands: U256 bitmask per row indicating which positions inside the band are 1
    // - start_pos[i]: left edge column of row i's band
    // - cols: total number of columns in the unknown vector u.
    // Throws std::runtime_error("ZeroRow i") if a row has no 1s inside the band.
    std::vector<FE> simple_gauss(std::vector<FE> y,
                                const std::vector<U256>& bands,
                                const std::vector<std::size_t>& start_pos,
                                std::size_t cols,
                                std::size_t band_width);

} // namespace rbokvs
