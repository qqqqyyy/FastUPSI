#include "tree/poly.h"
#include "utils.h"

using namespace oc;
using namespace upsi;

const int n = 20 * (1 << 10), m = 4;
bool DEBUG = true;

int main() {
    Poly polys[n];

    PRNG prng(oc::ZeroBlock);
    
    std::vector<oc::block> keys[n], values[n];
    std::vector<int> ind;
    std::vector<oc::block> rs;

    for (int i = 0; i < n; ++i) {
        keys[i] = GetRandomSet(&prng, 4);
        for (int j = 0; j < m; ++j) {
            values[i].push_back(GetRandomSetElement(&prng));
            // keys[i].push_back(toBlock(0, j + 1));
            // values[i].push_back(toBlock(0, j + 1));
        }
        ind.push_back(rand() % m);
    }

    Timer t0("polynomial interpolation");
    for (int i = 0; i < n; ++i) {
        polys[i].n = m;
        polys[i].interpolation(keys[i], values[i]);
    }
    t0.stop();

    // for (int i = 0; i < m; ++i) std::cout << polys[0].ase[i] << std::endl;
    Timer t1("eval");
    for (int i = 0; i < n; ++i) {
        rs.push_back(polys[i].eval1(keys[i][ind[i]]));
    }
    t1.stop();

    if(DEBUG) {
        for (int i = 0; i < n; ++i) if(rs[i] != values[i][ind[i]]) {
            std::cout << rs[i] << " " << values[i][ind[i]] << std::endl;
            throw std::runtime_error("!!!");
        }
    }

    return 0;
}