#include "rb_okvs.hpp"

#include <stdexcept>

namespace rbokvs {

    namespace {
        constexpr double EPSILON = 1.0;   // EPSILON
        constexpr std::size_t BAND_WIDTH = 80;    // BAND_WIDTH
    }

    RbOkvs::RbOkvs(std::size_t kv_count,
                const std::array<std::uint8_t,16>& r1,
                const std::array<std::uint8_t,16>& r2)
        : columns(static_cast<std::size_t>((1.0 + EPSILON) * static_cast<double>(kv_count))),
        bandWidth((BAND_WIDTH < columns) ? BAND_WIDTH : (columns * 80) / 100),
        r1(r1),
        r2(r2){

        if (columns == 0) {
            throw std::invalid_argument("RbOkvs: columns must be > 0");
        }
        if (bandWidth == 0 || bandWidth >= columns) {
            throw std::invalid_argument("RbOkvs: require 0 < band_width < columns");
        }
        if (bandWidth > 256) {
            throw std::invalid_argument("RbOkvs: band_width must be <= 256 (U256 mask)");
        }

    }

    std::vector<FE> RbOkvs::encode(const std::vector<std::pair<FE,FE>>& input) const{
        // Build sorted banded system
        auto [bands, start_pos, y] =
            create_sorted_matrix(columns, bandWidth, r1, r2, input);

        // Solve s for M * s = V
        return simple_gauss(std::move(y), bands, start_pos, columns, bandWidth);
    }

    std::vector<FE> RbOkvs::decode(const std::vector<FE>& encoding, const std::vector<FE>& keys) const{
        const std::size_t n = keys.size();
        std::vector<std::size_t> start(n);
        std::vector<U256> band(n);
        std::vector<FE> res(n, FE::zero());

        // precompute start & mask for each key
        for (std::size_t i = 0; i < n; ++i) {
            start[i] = hash_to_index(keys[i], r1, columns - bandWidth);
            band[i]  = hash_to_band (keys[i], r2);
        }

        // compute inner products
        for (std::size_t i = 0; i < n; ++i) {
            const auto si = start[i];
            if (si + bandWidth > encoding.size())
                throw std::out_of_range("RbOkvs::decode: window out of range");
            res[i] = inner_product(band[i], &encoding[si], bandWidth);
        }
        return res;
    }

}