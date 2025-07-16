#include "utils.h"

#include <chrono>
#include <iomanip>


namespace upsi {


////////////////////////////////////////////////////////////////////////////////
// COMPUTE HASH
////////////////////////////////////////////////////////////////////////////////

template<typename T>
BinaryHash computeBinaryHash(T &elem) {
    //TODO
	return elem;
}

// generate random binary hash
BinaryHash generateRandomHash(oc::PRNG* prng) {
	BinaryHash random_bytes; //TODO
    return random_bytes;
}

// generate random binary hash for cnt paths
std::vector<BinaryHash> generateRandomHash(oc::PRNG* prng, size_t cnt) {
    std::vector<BinaryHash> hsh;
	for (u64 i = 0; i < cnt; ++i) {
		hsh.push_back(generateRandomHash(prng));
	}
    return hsh;
}

Element GetRandomSetElement(oc::PRNG* prng) {
    Element elem = prng->get<oc::block>();
    if((elem & oc::OneBlock) == oc::OneBlock) elem ^= oc::OneBlock;
    return elem;
}

Element GetRandomPadElement(oc::PRNG* prng) {
    Element elem = prng->get<oc::block>();
    return elem | oc::OneBlock;
}

std::vector<Element> GetRandomSet(oc::PRNG* prng, size_t set_size) {
    std::vector<Element> elems;
    elems.reserve(set_size);
    for (u64 i = 0; i < set_size; ++i) elems.push_back(GetRandomSetElement(prng));
    return elems;
}

////////////////////////////////////////////////////////////////////////////////
// Timer
////////////////////////////////////////////////////////////////////////////////


Timer::Timer(std::string msg, std::string color) : message(msg), color(color) {
    start = std::chrono::high_resolution_clock::now();
}

void Timer::lap() {
    start = std::chrono::high_resolution_clock::now();
    using_laps = true;
}

void Timer::stop() {
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = stop - start;
    if (using_laps) {
        laps.push_back(elapsed);
    } else {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << color << message << " (s)\t: ";
        std::cout << elapsed.count() << RESET << std::endl;
    }
}

void Timer::print() {
    float average = 0;
    float min = std::numeric_limits<float>::max();
    float max = 0;
    for (const auto& lap : laps) {
        average += lap.count();
        if (lap.count() < min) { min = lap.count(); }
        if (lap.count() > max) { max = lap.count(); }
    }
    average /= laps.size();

    std::cout << std::fixed << std::setprecision(3);
    std::cout << color << message << " (s)\t: ";
    std::cout << average << " (AVG), ";
    std::cout << min << " (MIN), ";
    std::cout << max << " (MAX)" << RESET << std::endl;
}

}
