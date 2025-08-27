#include <cassert>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>

#include "types.hpp"   // using FE = fe252::FE; using U256 = std::array<uint64_t,4>
#include "utils.hpp"   // declarations under test

using rbokvs::FE;
using rbokvs::U256;

// ------------------------ helpers ------------------------

static FE fe_from_u64(uint64_t v) { return FE::from_u64(v); }

static void set_bit(U256& m, std::size_t idx) {
    if (idx >= 256) return;
    m[idx / 64] |= (1ULL << (idx % 64));
}

static std::array<uint8_t,16> salt_fill(uint8_t start) {
    std::array<uint8_t,16> r{};
    for (int i = 0; i < 16; ++i) r[i] = static_cast<uint8_t>(start + i);
    return r;
}

// Check monotonic nondecreasing for a vector
template<class T>
static bool is_non_decreasing(const std::vector<T>& v) {
    for (size_t i = 1; i < v.size(); ++i) {
        if (v[i] < v[i-1]) return false;
    }
    return true;
}

// ------------------------ tests ------------------------

static void test_hash_basic() {
    std::cout << "[hash] basic...\n";
    auto r1 = salt_fill(0x10);
    auto r2 = salt_fill(0x20);
    auto r3 = salt_fill(0x30);

    // deterministic
    FE x = FE::from_u64(123456789ull);
    size_t idx1 = rbokvs::hash_to_index(x, r1, 1000);
    size_t idx2 = rbokvs::hash_to_index(x, r1, 1000);
    assert(idx1 == idx2);
    assert(idx1 < 1000);

    // different salt => likely different (not guaranteed mathematically, but very likely)
    size_t idx3 = rbokvs::hash_to_index(x, salt_fill(0x11), 1000);
    // If it's equal by chance, it's still fine; just check range
    assert(idx3 < 1000);

    // range=0 is tolerated (returns 0)
    size_t idx0 = rbokvs::hash_to_index(x, r1, 0);
    assert(idx0 == 0);

    // band is a 256-bit mask (no structural property to assert except that it's deterministic)
    U256 m1 = rbokvs::hash_to_band(x, r2);
    U256 m2 = rbokvs::hash_to_band(x, r2);
    assert(std::memcmp(m1.data(), m2.data(), sizeof(U256)) == 0);

    // bin index in [0, numBins)
    size_t b7 = rbokvs::hash_to_bin(x, r3, 7);
    assert(b7 < 7);
    size_t b1 = rbokvs::hash_to_bin(x, r3, 1);
    assert(b1 == 0);

    std::cout << "  ok\n";
}

static void test_inner_product_small() {
    std::cout << "[inner_product] small cases...\n";
    // x = [0,1,2,3,4,5,6,7]
    FE x[8];
    for (int i = 0; i < 8; ++i) x[i] = FE::from_u64(static_cast<uint64_t>(i));

    // mask with bits {0,2,5}
    U256 m{};
    set_bit(m, 0);
    set_bit(m, 2);
    set_bit(m, 5);

    FE s = rbokvs::inner_product(m, x, 8);
    FE expected = x[0] + x[2] + x[5];
    assert(s == expected);

    // if len < highest bit set, function clamps to len
    FE s2 = rbokvs::inner_product(m, x, 3); // only bits 0..2 seen -> 0+2
    assert(s2 == (x[0] + x[2]));

    // test across limb boundary
    FE longx[130];
    for (int i = 0; i < 130; ++i) longx[i] = FE::from_u64(i + 1);
    U256 m2{};
    set_bit(m2, 63);   // last bit of limb 0
    set_bit(m2, 64);   // first bit of limb 1
    set_bit(m2, 127);  // last bit of limb 1
    FE s3 = rbokvs::inner_product(m2, longx, 130);
    FE exp3 = longx[63] + longx[64] + longx[127];
    assert(s3 == exp3);

    std::cout << "  ok\n";
}

static void test_create_sorted_matrix_order() {
    std::cout << "[create_sorted_matrix] order & mapping...\n";
    const std::size_t n = 64;
    const std::size_t cols = 200;
    const std::size_t bw = 80;
    auto r1 = salt_fill(0x01);
    auto r2 = salt_fill(0x11);

    // deterministic inputs
    std::vector<std::pair<FE,FE>> input;
    input.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        input.emplace_back(FE::from_u64(1000 + i), FE::from_u64(5000 + i));
    }

    auto [M, start_ids, y] = rbokvs::create_sorted_matrix(cols, bw, r1, r2, input);

    // 1) start_ids should be non-decreasing
    assert(is_non_decreasing(start_ids));

    // 2) y[i] should be input[idx].second where (idx, start) sorted by start
    std::vector<std::pair<size_t,size_t>> pairs;
    pairs.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        size_t s = rbokvs::hash_to_index(input[i].first, r1, cols - bw);
        pairs.emplace_back(i, s);
    }
    std::stable_sort(pairs.begin(), pairs.end(),
                     [](auto a, auto b){ return a.second < b.second; });

    for (size_t i = 0; i < n; ++i) {
        size_t idx = pairs[i].first;
        assert(y[i] == input[idx].second);
        // 3) matrix row equals hash_to_band of the corresponding key
        U256 expected = rbokvs::hash_to_band(input[idx].first, r2);
        assert(std::memcmp(M[i].data(), expected.data(), sizeof(U256)) == 0);
        // 4) start_ids[i] equals the sorted start
        assert(start_ids[i] == pairs[i].second);
    }

    std::cout << "  ok\n";
}

static void test_simple_gauss_constructed() {
    std::cout << "[simple_gauss] constructed triangular system...\n";
    // cols=10, bw=5, rows=4. Each row has a single 1 at local index 0.
    const std::size_t cols = 10, bw = 5, rows = 4;

    std::vector<U256> bands(rows);
    std::vector<std::size_t> start(rows);
    std::vector<FE> y(rows);

    // choose start positions so pivots are at 0,1,2,3
    for (size_t i = 0; i < rows; ++i) {
        U256 m{};
        set_bit(m, 0);         // local index 0 -> pivot at start[i]
        bands[i] = m;
        start[i] = i;          // pivot column = i
        y[i] = FE::from_u64(100 + i);
    }

    auto x = rbokvs::simple_gauss(y, bands, start, cols, bw);
    // Only pivots get set to y[i]
    for (size_t i = 0; i < rows; ++i) {
        assert(x[i] == FE::from_u64(100 + i));
    }
    // others remain zero
    for (size_t i = rows; i < cols; ++i) {
        assert(x[i] == FE::zero());
    }

    std::cout << "  ok\n";
}

static void test_simple_gauss_zero_row_throws() {
    std::cout << "[simple_gauss] zero row throws...\n";
    const std::size_t cols = 16, bw = 8, rows = 3;

    std::vector<U256> bands(rows);
    std::vector<std::size_t> start(rows);
    std::vector<FE> y(rows, FE::one());

    // row 0 has no 1s in its band -> should throw
    bands[0] = U256{0,0,0,0};
    start[0] = 0;

    // row 1,2 valid single-1 rows
    bands[1] = U256{0,0,0,0}; set_bit(bands[1], 0); start[1] = 1;
    bands[2] = U256{0,0,0,0}; set_bit(bands[2], 2); start[2] = 3;

    bool threw = false;
    try {
        (void)rbokvs::simple_gauss(y, bands, start, cols, bw);
    } catch (const std::runtime_error& e) {
        threw = true;
    }
    assert(threw);
    std::cout << "  ok\n";
}

static void test_full_pipeline_random() {
    std::cout << "[pipeline] matrix -> solve -> verify inner products...\n";
    const std::size_t n = 256;
    const std::size_t cols = static_cast<std::size_t>((1.0 + 1.0) * double(n)); // EPSILON=1.0
    const std::size_t bw = 80; // ≤ 256 (required by our U256 mask)
    auto r1 = salt_fill(0xAA);
    auto r2 = salt_fill(0xBB);

    std::mt19937_64 rng(123456);
    std::vector<std::pair<FE,FE>> input;
    input.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        uint64_t k = rng();
        uint64_t v = rng();
        input.emplace_back(FE::from_u64(k), FE::from_u64(v));
    }

    // Build & solve
    auto [M, start_ids, y] = rbokvs::create_sorted_matrix(cols, bw, r1, r2, input);
    auto encoding = rbokvs::simple_gauss(y, M, start_ids, cols, bw);

    // Verify: for each original pair, recompute start/band and dot against encoding window
    for (size_t i = 0; i < n; ++i) {
        size_t s = rbokvs::hash_to_index(input[i].first, r1, cols - bw);
        U256 m   = rbokvs::hash_to_band(input[i].first, r2);
        FE got   = rbokvs::inner_product(m, &encoding[s], bw);
        FE want  = input[i].second;
        if (got != want) {
            std::cerr << "  mismatch at i=" << i << "\n";
            assert(false);
        }
    }

    std::cout << "  ok\n";
}

int main() {
    try {
        test_hash_basic();
        test_inner_product_small();
        test_create_sorted_matrix_order();
        test_simple_gauss_constructed();
        test_simple_gauss_zero_row_throws();
        test_full_pipeline_random();
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    std::cout << "All utils tests passed.\n";
    return 0;
}
