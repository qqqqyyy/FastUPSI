#include "field_stark252.hpp"
#include <cassert>
#include <iostream>

int main() {
    using FE = fe252::FE;

    auto z = FE::zero(), o = FE::one();
    assert(z != o);

    FE a = FE::from_u64(123), b = FE::from_u64(456);
    FE c = a + b;
    FE d = a * b;

    // inverse
    FE ai = a.inv();
    assert(a * ai == o);

    // additive identity
    assert((a - a).is_zero());

    // bytes round-trip (compare canonical bytes)
    auto bytes = c.to_bytes_le();
    FE c2 = FE::from_bytes_le(bytes.data(), bytes.size());
    assert(c.to_bytes_le() == c2.to_bytes_le());

    std::cout << "CryptoPP FE tests passed.\n";
    return 0;
}