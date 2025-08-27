#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>

#include "types.hpp"
#include "utils.hpp"

namespace rbokvs {

    class RbOkvs {
        public:
            std::size_t columns = 0;

            RbOkvs(std::size_t kvCount,
                const std::array<std::uint8_t, 16>& inputR1,
                const std::array<std::uint8_t, 16>& inputR2);

            //public functions
            std::vector<FE> encode(const std::vector<std::pair<FE,FE>>& input) const;
            std::vector<FE> decode(const std::vector<FE>& encoding,
                                    const std::vector<FE>& keys) const;

            //not sure if needed
            std::size_t getBandWidth() const noexcept { return bandWidth; }
            const std::array<std::uint8_t, 16>& getR1() const noexcept { return r1; }
            const std::array<std::uint8_t, 16>& getR2() const noexcept { return r2; }

        private:
            std::size_t bandWidth;
            std::array<std::uint8_t, 16> r1;
            std::array<std::uint8_t, 16> r2;
    };

}