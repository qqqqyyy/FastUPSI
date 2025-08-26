#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

// your FE (Crypto++ backend)
#include "field_stark252.hpp"

namespace rbokvs {

// Field element type used everywhere in OKVS
using FE = fe252::FE;

// Simple 256-bit mask (little-endian limbs, like your Rust U256.0[0..3])
using U256 = std::array<std::uint64_t, 4>;

// Convenience alias for input pairs
template <class K, class V>
using Pair = std::pair<K, V>;

}