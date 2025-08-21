#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace rbokvs {

    inline constexpr double epsilon = 1.0;          // EPSILON
    inline constexpr std::size_t defaultBandWidth = 80; // BAND_WIDTH

    class RbOkvs {
        public:
            std::size_t columns = 0;

            RbOkvs(std::size_t kvCount,
                const std::array<std::uint8_t, 16>& inputR1,
                const std::array<std::uint8_t, 16>& inputR2)
                : columns(static_cast<std::size_t>((1.0 + epsilon) * static_cast<double>(kvCount))),
                bandWidth((defaultBandWidth < columns)
                                ? defaultBandWidth
                                : (columns * 80) / 100),
                r1(inputR1),
                r2(inputR2)
            {}

            std::size_t getBandWidth() const noexcept { return bandWidth; }
            const std::array<std::uint8_t, 16>& getR1() const noexcept { return r1; }
            const std::array<std::uint8_t, 16>& getR2() const noexcept { return r2; }

        private:
            std::size_t bandWidth;
            std::array<std::uint8_t, 16> r1;
            std::array<std::uint8_t, 16> r2;
    };

}