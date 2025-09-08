#include "utils.h"
#include "ASE/poly.h"

using namespace oc;
using namespace upsi;

const int n = 20 * (1 << 10) * 2, m = 4;
bool DEBUG = true;

int main() {
    std::vector<Poly> polys;

    PRNG prng(oc::ZeroBlock);
    
    std::vector<BlockVec> keys, values;
    std::vector<int> ind;
    std::vector<oc::block> rs;

    for (int i = 0; i < n; ++i) {
        keys.push_back(GetRandomSet(&prng, 4));
        BlockVec cur_val;
        for (int j = 0; j < m; ++j) {
            cur_val.push_back(GetRandomSetElement(&prng));
            // keys[i].push_back(toBlock(0, j + 1));
            // values[i].push_back(toBlock(0, j + 1));
        }
        values.push_back(cur_val);
        ind.push_back(rand() % m);
    }

    Timer t0("polynomial interpolation");
    t0.setTimePoint("start");
    for (int i = 0; i < n; ++i) {
        polys.push_back(Poly(m));
        // polys[i].interpolation(keys[i], values[i]);
    }
    batchInterpolation(polys, keys, values);
    t0.setTimePoint("end");

    // for (int i = 0; i < m; ++i) std::cout << polys[0].ase[i] << std::endl;
    Timer t1("eval");
    t1.setTimePoint("start");
    for (int i = 0; i < n; ++i) {
        rs.push_back(polys[i].eval1(keys[i][ind[i]]));
    }
    t1.setTimePoint("end");

    std::cout << t0 << "\n" << t1 << "\n";

    if(DEBUG) {
        // for (int i = 0; i < n; ++i) if(rs[i] != values[i][ind[i]]) {
        //     std::cout << rs[i] << " " << values[i][ind[i]] << std::endl;
        //     throw std::runtime_error("!!!");
        // }
        for (int i = 0; i < n; ++i) for (int j = 0; j < m; ++j) {
            if(polys[i].eval1(keys[i][j]) != values[i][j]) {
                throw std::runtime_error("!!!");
            }
        }
    }

    return 0;
}