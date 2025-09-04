#include "rb_okvs_gf2.hpp"
#include <stdexcept>
#include <algorithm>

namespace rbokvs {

    namespace {
        constexpr double EPSILON = 1.0;
        constexpr std::size_t BAND_WIDTH_CAP = 256;
        constexpr std::size_t DEFAULT_BW = 80;
    }

    RbOkvsGF2::RbOkvsGF2(std::size_t kv_count,
                         const std::array<std::uint8_t,16>& r1_,
                         const std::array<std::uint8_t,16>& r2_)
        : columns(static_cast<std::size_t>((1.0 + EPSILON) * static_cast<double>(kv_count))),
          bandWidth(0),
          r1(r1_),
          r2(r2_)
    {
        if (columns == 0) throw std::invalid_argument("RbOkvsGF2: columns must be > 0");
        std::size_t bw = (columns >= DEFAULT_BW) ? DEFAULT_BW : std::max<std::size_t>(1, (columns * 80) / 100);
        bandWidth = std::min<std::size_t>(bw, BAND_WIDTH_CAP);
        if (bandWidth >= columns) bandWidth = std::max<std::size_t>(1, columns - 1);
    }

    std::vector<FE2> RbOkvsGF2::encode(const std::vector<std::pair<FE2,FE2>>& input) const {
        auto [bands, start_pos, y] = create_sorted_matrix(columns, bandWidth, r1, r2, input);
        return simple_gauss(std::move(y), bands, start_pos, columns, bandWidth);
    }

    std::vector<FE2> RbOkvsGF2::decode(const std::vector<FE2>& encoding, const std::vector<FE2>& keys) const {
        const std::size_t n = keys.size();
        std::vector<std::size_t> start(n);
        std::vector<U256> band(n);
        std::vector<FE2> res(n, FE2::zero());

        for (std::size_t i = 0; i < n; ++i) {
            start[i] = hash_to_index(keys[i], r1, columns - bandWidth);
            band[i]  = hash_to_band (keys[i], r2);
        }

        for (std::size_t i = 0; i < n; ++i) {
            const auto si = start[i];
            if (si + bandWidth > encoding.size())
                throw std::out_of_range("RbOkvsGF2::decode: window out of range");
            res[i] = inner_product(band[i], &encoding[si], bandWidth);
        }
        return res;
    }

} 
