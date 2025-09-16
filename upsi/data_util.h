#ifndef DATA_UTIL_H
#define DATA_UTIL_H

#include <fstream>
#include "utils.h"

namespace upsi{

class Dataset{

public:
    int start_size;
    int days;
    int add_size; // daily addition size
    int del_size; // daily deletion size
    std::vector<Element> initial_set;
    std::vector<Element> intersection;
    std::vector<std::vector<Element> > daily_addition;
    std::vector<std::vector<Element> > daily_deletion;

    Dataset(){}
    Dataset(int start_size, int days, int add_size, int del_size) {
        this->start_size = start_size;
        this->days = days;
        this->add_size = add_size;
        this->del_size = del_size;
    }

    void print() {
        std::cout << "[Dataset] initial set:\n";
        for(const auto& elem: initial_set) std::cout << elem << " ";
        std::cout << "\n";
        std::cout << "[Dataset] intersection:\n";
        for(const auto& elem: intersection) std::cout << elem << " ";
        std::cout << "\n\n";

        for (int i = 0; i < days; ++i) {
            std::cout << "[Dataset] day " << std::to_string(i) << ":\n";
            std::cout << "[Dataset] deletion:\n";
            for(const auto& elem: daily_deletion[i]) std::cout << elem << " ";
            std::cout << "\n";
            std::cout << "[Dataset] addition:\n";
            for(const auto& elem: daily_addition[i]) std::cout << elem << " ";
            std::cout << "\n\n";
        }
    }

    static void write_vec(std::ostream& os, const BlockVec& v) {
        os << v.size() << '\n';
        for (const auto& e : v) os.write(reinterpret_cast<const char*>(e.data()), 16);
        os << '\n';
    }
    static void read_vec(std::istream& is, BlockVec& v) {
        std::size_t n;
        if (!(is >> n)) throw std::runtime_error("[Dataset] bad vec size");
        is.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        v.resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            std::array<unsigned char,16> buf{};
            is.read(reinterpret_cast<char*>(buf.data()), 16);
            if (!is) throw std::runtime_error("[Dataset] bad vec elem");
            v[i] = oc::block(buf);
        }
    }

    void Read(const std::string& fn) {
        std::ifstream file(fn, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("[Dataset] failed to open " + fn);
        }

        if (!(file >> start_size >> days >> add_size >> del_size))
            throw std::runtime_error("[Dataset] bad params");

        std::cout << "[Dataset] Read: " << "start_size = " << start_size << ", days = " << days 
        << ",\n\t\t add_size = " << add_size << ", del_size = " << del_size << "\n";

        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        initial_set.clear(); intersection.clear();
        daily_addition.resize(days);
        daily_deletion.resize(days);

        std::cout << "[Dataset] Reading initial sets...\n";

        read_vec(file, initial_set);
        read_vec(file, intersection);

        std::cout << "[Dataset] Reading daily sets...\n";

        for (std::size_t i = 0; i < days; ++i) read_vec(file, daily_addition[i]);
        for (std::size_t i = 0; i < days; ++i) read_vec(file, daily_deletion[i]);

        std::cout << "[Dataset] done.\n\n";

        file.close();
        if (file.is_open()) {
            throw std::runtime_error("[Dataset] failed to close " + fn);
        }
    }

    void Write(const std::string& fn) const {
        std::ofstream file(fn, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("[Dataset] failed to open " + fn);
        }

        file << start_size << ' ' << days << ' ' << add_size << ' ' << del_size << '\n';

        write_vec(file, initial_set);
        write_vec(file, intersection);

        for (const auto& day : daily_addition) write_vec(file, day);
        for (const auto& day : daily_deletion) write_vec(file, day);

        file.close();
        if (file.fail()) {
            throw std::runtime_error("[Dataset] failed to write and close " + fn);
        }
    }
};

inline Element sample_and_delete(std::vector<Element>& vec, oc::PRNG& prng) {
    size_t idx = prng.get<size_t>() % vec.size();
    Element rs = vec[idx];
    if(idx != vec.size() - 1) vec[idx] = vec.back();
    vec.pop_back();
    return rs;
}

std::pair<Dataset, Dataset> GenerateSets(int start_size, int days, int add_size, int del_size);

} // namespace upsi

#endif