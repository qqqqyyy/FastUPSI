#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "field_stark252.hpp"

namespace rbokvs {

    using FE = fe252::FE;

    using U256 = std::array<std::uint64_t, 4>;

    template <class K, class V>
    using Pair = std::pair<K, V>;

}