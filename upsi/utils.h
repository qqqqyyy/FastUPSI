#ifndef UTILS_H
#define UTILS_H

#include <string>

#include <algorithm>
#include <bitset>
#include <chrono>
#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Common/BitVector.h"
#include "cryptoTools/Crypto/RandomOracle.h"

// for printing to the command line in colors
#define RED    "\033[0;31m"
#define GREEN  "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE   "\033[0;34m"
#define CYAN   "\033[0;36m"
#define WHITE  "\033[0;37m"
#define RESET  "\033[0m"

namespace upsi {

    // #define CURVE_ID NID_X9_62_prime256v1
	#define DEFAULT_NODE_SIZE 4
    #define DEFAULT_STASH_SIZE 89
    #define DEFAULT_ADAPTIVE_SIZE (1 << 10)
    #define DEFAULT_VOLE_SIZE (1 << 14)

    // type of elements in each party's sets
    // TODO
	typedef oc::block Element;
    typedef oc::u64 u64;
    typedef std::vector<oc::block> BlockVec;
    typedef std::span<oc::block> BlockSpan;
    typedef std::shared_ptr<oc::block> BlockPtr;
    typedef std::vector<BlockPtr> PtrVec;
    typedef std::pair<oc::block, oc::block> OPRFValue;
    typedef std::vector<OPRFValue> OPRFValueVec;

    // inline size_t COMM = 0; //communication, number of bytes

    // protocol functionality options
    // enum Functionality { PSI };


    // template<typename T> T elementCopy(const T &elem);
    
    typedef oc::BitVector BinaryHash;

    BinaryHash block2binary(oc::block x);
	BinaryHash computeBinaryHash(oc::block elem, oc::block seed);
    BinaryHash generateRandomHash(oc::PRNG* prng);
	std::vector<BinaryHash> generateRandomHash(oc::PRNG* prng, size_t cnt);

    Element GetRandomSetElement(oc::PRNG* prng);
    // Element GetRandomPadElement(oc::PRNG* prng);
    std::vector<Element> GetRandomSet(oc::PRNG* prng, size_t set_size);

    //random oracle
    oc::block random_oracle(oc::block x, oc::block seed = oc::ZeroBlock);
    OPRFValue random_oracle_256(oc::block x, size_t index, oc::block seed = oc::ZeroBlock);

    template<typename type> void random_shuffle(std::vector<type>& vec);

    /**
     * class to unify time benchmarking
     */
    // class Timer {
    //     public:
    //         Timer(std::string msg, std::string color = WHITE);
    //         void stop();
    //         void lap();
    //         void print();
    //     private:
    //         std::string message;
    //         std::string color;
    //         std::chrono::time_point<std::chrono::high_resolution_clock> start;
    //         std::vector<std::chrono::duration<float>> laps;
    //         bool using_laps = false;
    // };
}

#endif
