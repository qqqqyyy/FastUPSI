#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "fe2_128.hpp"
#include "utils_gf2.hpp"
#include "types.hpp"

namespace rbokvs {

class RbOkvsGF2 {
public:
    RbOkvsGF2(std::size_t kv_count,
              const std::array<std::uint8_t,16>& r1,
              const std::array<std::uint8_t,16>& r2);

    std::vector<FE2> encode(const std::vector<std::pair<FE2,FE2>>& input) const;
    std::vector<FE2> decode(const std::vector<FE2>& encoding, const std::vector<FE2>& keys) const;

    std::size_t columns;
    std::size_t bandWidth;

private:
    std::array<std::uint8_t,16> r1;
    std::array<std::uint8_t,16> r2;
};

} // namespace rbokvs
