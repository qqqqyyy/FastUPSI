#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdexcept>

namespace rbokvs {

// GF(2^128) with irreducible polynomial: x^128 + x^7 + x^2 + x + 1 (0x87 reduction)
class FE2_128 {
public:
    using limb_t = std::uint64_t;

    FE2_128() : lo_(0), hi_(0) {}
    FE2_128(limb_t lo, limb_t hi) : lo_(lo), hi_(hi) {}
    explicit FE2_128(std::uint64_t v) : lo_(v), hi_(0) {}

    static inline FE2_128 zero() { return FE2_128(0, 0); }
    static inline FE2_128 one()  { return FE2_128(1, 0); }

    inline limb_t lo() const { return lo_; }
    inline limb_t hi() const { return hi_; }

    inline bool operator==(const FE2_128& other) const { return lo_==other.lo_ && hi_==other.hi_; }
    inline bool operator!=(const FE2_128& other) const { return !(*this==other); }

    inline FE2_128& operator+=(const FE2_128& rhs) { lo_ ^= rhs.lo_; hi_ ^= rhs.hi_; return *this; }
    inline FE2_128& operator-=(const FE2_128& rhs) { return (*this += rhs); }
    inline friend FE2_128 operator+(FE2_128 a, const FE2_128& b){ a+=b; return a; }
    inline friend FE2_128 operator-(FE2_128 a, const FE2_128& b){ a-=b; return a; }

    inline FE2_128& operator*=(const FE2_128& rhs){ *this = (*this * rhs); return *this; }
    inline friend FE2_128 operator*(const FE2_128& a, const FE2_128& b){
        FE2_128 x=a, y=b, z=FE2_128::zero();
        for(int i=0;i<128;++i){
            if(y.lo_ & 1ULL){ z.lo_^=x.lo_; z.hi_^=x.hi_; }
            bool msb = (x.hi_ & (1ULL<<63))!=0;
            std::uint64_t new_hi = (x.hi_<<1) | (x.lo_>>63);
            std::uint64_t new_lo = (x.lo_<<1);
            if(msb){ new_lo ^= 0x87ULL; } // reduction by x^128 + x^7 + x^2 + x + 1
            x.hi_=new_hi; x.lo_=new_lo;
            y.lo_ = (y.lo_>>1) | (y.hi_<<63);
            y.hi_ >>= 1;
        }
        return z;
    }

    inline FE2_128 inv() const {
        if(*this==FE2_128::zero()) throw std::domain_error("FE2_128::inv(): zero has no inverse");
        FE2_128 pow2i = *this;     // a^{2^0}
        FE2_128 acc   = FE2_128::one();
        for(int i=1;i<=127;++i){
            pow2i = square(pow2i); // a^{2^i}
            acc   = acc * pow2i;   // Π_{i=1}^{127} a^{2^i} = a^{2^128-2}
        }
        return acc;
    }

    static inline FE2_128 square(const FE2_128& x){ return x * x; }

    inline std::array<std::uint8_t,16> to_bytes_le() const {
        std::array<std::uint8_t,16> out{};
        std::memcpy(out.data()+0, &lo_, 8);
        std::memcpy(out.data()+8, &hi_, 8);
        return out;
    }

private:
    limb_t lo_, hi_;
};

} 
