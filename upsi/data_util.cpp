#include "data_util.h"

namespace upsi{

std::pair<Dataset, Dataset> GenerateSets(int start_size, int days, int add_size, int del_size) {
    std::cout << "[Generate Dataset] " << "start_size = " << start_size << ", days = " << days 
        << ",\n\t\t\t add_size = " << add_size << ", del_size = " << del_size << "\n";
    oc::PRNG prng(oc::sysRandomSeed());
    Dataset X(start_size, days, add_size, del_size), Y(start_size, days, add_size, del_size);


    int initial_I_size = start_size / 4;
    int initial_daily_I = std::fmin((start_size - initial_I_size) / 2, ((add_size - del_size) * days) / 5);
    int I_size = ((add_size - del_size) * days) / 40;


    auto universe = GetRandomSet(&prng, start_size * 2 + add_size * days * 2 - initial_I_size - I_size + 100);
    std::sort(universe.begin(), universe.end());
    universe.erase(std::unique(universe.begin(), universe.end()), universe.end());
    random_shuffle<oc::block>(universe);
    

    int universe_size = universe.size();

    X.initial_set.reserve(start_size);
    Y.initial_set.reserve(start_size);
    X.intersection.reserve(initial_I_size);
    Y.intersection.reserve(initial_I_size);
    X.daily_addition.resize(days);
    Y.daily_addition.resize(days);
    X.daily_deletion.resize(days);
    Y.daily_deletion.resize(days);

    X.intersection.insert(X.intersection.end(), universe.begin(), universe.begin() + initial_I_size);
    Y.intersection.insert(Y.intersection.end(), universe.begin(), universe.begin() + initial_I_size);

    X.initial_set = X.intersection;
    Y.initial_set = Y.intersection;

    int idx = initial_I_size;
    for (int i = 0; i < start_size - initial_I_size; ++i) {
        X.initial_set.push_back(universe[idx++]);
        Y.initial_set.push_back(universe[idx++]);
    }


    std::vector<Element> X_universe, Y_universe;

    for (int i = 0; i < initial_daily_I; ++i) {
        X_universe.push_back(Y.initial_set[start_size - i - 1]);
        Y_universe.push_back(X.initial_set[start_size - i - 1]);
    }

    for (int i = 0; i < I_size; ++i) {
        X_universe.push_back(universe[idx]);
        Y_universe.push_back(universe[idx++]);
    }

    while(idx + 1 < universe_size) {
        X_universe.push_back(universe[idx++]);
        Y_universe.push_back(universe[idx++]);
    }

    random_shuffle<oc::block>(X_universe);
    random_shuffle<oc::block>(Y_universe);

    if(del_size == 0) {
        int idx_X = 0, idx_Y = 0;
        for (int i = 0; i < days; ++i) {
            X.daily_addition[i].reserve(add_size);
            Y.daily_addition[i].reserve(add_size);
            for (int j = 0; j < add_size; ++j) {
                X.daily_addition[i].push_back(X_universe[idx_X++]);
                Y.daily_addition[i].push_back(Y_universe[idx_Y++]);
            }
        }
    }
    else {
        oc::PRNG prng(oc::sysRandomSeed());
        std::vector<Element> cur_X = X.initial_set;
        std::vector<Element> cur_Y = Y.initial_set;
        for (int i = 0; i < days; ++i) {
            X.daily_deletion[i].reserve(del_size);
            Y.daily_deletion[i].reserve(del_size);
            for (int j = 0; j < del_size; ++j) {
                Element tmp0 = sample_and_delete(cur_X, prng);
                Element tmp1 = sample_and_delete(cur_Y, prng);
                X.daily_deletion[i].push_back(tmp0);
                X_universe.push_back(tmp0);
                Y.daily_deletion[i].push_back(tmp1);
                Y_universe.push_back(tmp1);
            }
            X.daily_addition[i].reserve(add_size);
            Y.daily_addition[i].reserve(add_size);
            for (int j = 0; j < add_size; ++j) {
                Element tmp0 = sample_and_delete(X_universe, prng);
                Element tmp1 = sample_and_delete(Y_universe, prng);
                X.daily_addition[i].push_back(tmp0);
                cur_X.push_back(tmp0);
                Y.daily_addition[i].push_back(tmp1);
                cur_Y.push_back(tmp1);
            }
        }
    }


    random_shuffle<oc::block>(X.initial_set);
    random_shuffle<oc::block>(Y.initial_set);

    std::cout << "[Generate Dataset] done.\n\n";

    return std::make_pair(X, Y);
}

}