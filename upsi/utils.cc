#include "utils.h"

#include <chrono>
#include <iomanip>

#include "upsi/network/upsi.pb.h"
// #include "upsi/util/elgamal_proto_util.h"


namespace upsi {


bool AbslParseFlag(absl::string_view text, Functionality* func, std::string* err) {
    if (text == "PSI") { *func = Functionality::PSI; }
    else {
        *err = "unknown functionality";
        return false;
    }
    return true;
}

std::string AbslUnparseFlag(Functionality func) {
    switch (func) {
        case Functionality::PSI:
            return "PSI";
        default:
            return "PSI";
    }
}

template<>
Element elementCopy(const Element& elem) {
	Element copy(elem);
	return copy;
}


////////////////////////////////////////////////////////////////////////////////
// COMPUTE BINARY HASH
////////////////////////////////////////////////////////////////////////////////

//convert byte hash to binary hash
std::string Byte2Binary(const std::string &byte_hash) {
    std::string binary_hash = "";
    for (char const &c: byte_hash) {
        binary_hash += std::bitset<8>(c).to_string();
    }
    return binary_hash;
}

template<typename T>
BinaryHash computeBinaryHash(T &elem) {
	return elem;
}

// generate random binary hash
BinaryHash generateRandomHash() {
	// Context ctx;
	// std::string random_bytes = ctx.GenerateRandomBytes(32); // 32 bytes for SHA256 => obtain random_path as a byte string
	std::string random_bytes; //TODO
    return random_bytes;
}

// generate random binary hash for cnt paths
void generateRandomHash(int cnt, std::vector<std::string> &hsh) {
	for (int i = 0; i < cnt; ++i) {
		hsh.push_back(generateRandomHash());
	}
}

int64_t NumericString2uint(const std::string &str) { //str should be fixed length
	int64_t x = 0;
	int len = str.length();
	for (int i = 0; i < len; ++i) {
		x = x * 10 + str[i] - '0';
	}
	return x;
}

std::string GetRandomNumericString(size_t length, bool padding) {
	std::string output;
    if (padding) {
		absl::StrAppend(&output, "1");
    } else {
		absl::StrAppend(&output, "0");
    }
	for (size_t i = 1; i < length; i++) {
		std::string next_char(1, rand() % 10 + '0');
		absl::StrAppend(&output, next_char);
	}
	return output;
}

std::string GetRandomSetElement() {
    return GetRandomNumericString(ELEMENT_STR_LENGTH, false);
}

// Element GetRandomPadElement(Context* ctx) {
//     return ctx->CreateBigNum(NumericString2uint(
//         GetRandomNumericString(ELEMENT_STR_LENGTH, true)
//     ));
// }

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
