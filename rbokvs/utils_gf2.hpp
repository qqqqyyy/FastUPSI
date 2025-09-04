#pragma once
#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

#include "types.hpp"      
#include "fe2_128.hpp"   

namespace rbokvs {

using FE2 = FE2_128;

std::size_t hash_to_index(const FE2& x, const std::array<std::uint8_t,16>& r1, std::size_t range);
U256        hash_to_band (const FE2& x, const std::array<std::uint8_t,16>& r2);
std::size_t hash_to_bin  (const FE2& x, const std::array<std::uint8_t,16>& r3, std::size_t numBins);

FE2 inner_product(const U256& m, const FE2* x, std::size_t len);

std::tuple<std::vector<U256>, std::vector<std::size_t>, std::vector<FE2>>
create_sorted_matrix(std::size_t columns,
                     std::size_t band_width,
                     const std::array<std::uint8_t,16>& r1,
                     const std::array<std::uint8_t,16>& r2,
                     const std::vector<std::pair<FE2,FE2>>& input);

std::vector<FE2> simple_gauss(std::vector<FE2> y,
                              const std::vector<U256>& bands,
                              const std::vector<std::size_t>& start_pos,
                              std::size_t cols,
                              std::size_t band_width);

} 
