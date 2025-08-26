#pragma once
#include <cryptopp/integer.h>
#include <array>
#include <cstdint>
#include <stdexcept>

namespace fe252 {

class FE {
    CryptoPP::Integer value;

    static const CryptoPP::Integer& prime() {
        static const CryptoPP::Integer prime(
          "36185027886661311069865932815214971204146870208012676262330495027512497617");
        return prime;
    }

    static CryptoPP::Integer mod(CryptoPP::Integer x) {
        x %= prime();
        if (x.IsNegative()) x += prime();
        return x;
    }

    public:
        FE() : value(CryptoPP::Integer::Zero()) {}
        explicit FE(const CryptoPP::Integer& x) : value(mod(x)) {}

        static FE zero() { return FE(CryptoPP::Integer::Zero()); }
        static FE one()  { return FE(CryptoPP::Integer::One());  }
        static FE from_u64(uint64_t x) { return FE(CryptoPP::Integer(x)); }

        FE  operator+(const FE& x) const { return FE(value + x.value); }
        FE  operator-(const FE& x) const { return FE(value - x.value); }
        FE  operator*(const FE& x) const { return FE((value * x.value) % prime()); }

        FE& operator+=(const FE& x) { value = mod(value + x.value); return *this; }
        FE& operator-=(const FE& x) { value = mod(value - x.value); return *this; }
        FE& operator*=(const FE& x) { value = (value * x.value) % prime();   return *this; }

        bool operator==(const FE& x) const { return value == x.value; }
        bool operator!=(const FE& x) const { return value != x.value; }

        bool isZero() const { return value.IsZero(); }

        FE inv() const {
            if (isZero()) throw std::domain_error("FE::inv of zero");
            return FE(value.InverseMod(prime()));
        }

        std::array<uint8_t, 32> to_bytes_le() const {
            std::array<uint8_t, 32> out{};
            CryptoPP::Integer t = value;
            const CryptoPP::Integer mask = (CryptoPP::Integer::One() << 8) - CryptoPP::Integer::One();
            for (size_t i = 0; i < out.size(); ++i) {
                if (t.IsZero()) { out[i] = 0; continue; }
                out[i] = static_cast<uint8_t>((t & mask).ConvertToLong());
                t >>= 8;
            }
            return out;
        }

        static FE from_bytes_le(const uint8_t* data, size_t len) {
            CryptoPP::Integer x = CryptoPP::Integer::Zero();
            const size_t n = (len > 32) ? 32 : len;
            for (size_t i = 0; i < n; ++i) {
                x += CryptoPP::Integer(data[i]) << (8 * i);
            }
            return FE(x);
        }

        const CryptoPP::Integer& raw() const { return value; }
    };

}
