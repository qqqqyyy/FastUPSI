#include "utils.h"
#include "ASE/poly.h"

using namespace oc;
using namespace upsi;

const int n = 20 * (1 << 10) * 2, m = 4;
bool DEBUG = true;

void one_run() {
    std::vector<Poly> polys;

    PRNG prng(oc::ZeroBlock);
    
    std::vector<BlockVec> keys, values;
    std::vector<int> ind;
    std::vector<oc::block> rs;

    for (int i = 0; i < n; ++i) {
        int cnt = prng.get<u8>() % m;
        // std::cout << cnt << std::endl;
        if(cnt) keys.push_back(GetRandomSet(&prng, cnt));
        else keys.push_back(BlockVec());
        BlockVec cur_val;
        for (int j = 0; j < cnt; ++j) {
            cur_val.push_back(GetRandomSetElement(&prng));
            // keys[i].push_back(toBlock(0, j + 1));
            // values[i].push_back(toBlock(0, j + 1));
        }
        values.push_back(cur_val);
        if(cnt) ind.push_back(rand() % cnt);
        else ind.push_back(0);
    }


    // std::cout << "polynomial interpolation ...\n";

    Timer t0("polynomial interpolation");
    t0.setTimePoint("start");
    for (int i = 0; i < n; ++i) {
        polys.push_back(Poly(m));
        // polys[i].interpolation(keys[i], values[i]);
    }
    batchInterpolation(polys, keys, values);
    t0.setTimePoint("end");


    // std::cout << "done ...\n";

    // for (int i = 0; i < m; ++i) std::cout << polys[0].ase[i] << std::endl;
    Timer t1("eval");
    t1.setTimePoint("start");
    for (int i = 0; i < n; ++i) {
        if(keys[i].size()) rs.push_back(polys[i].eval1(keys[i][ind[i]]));
        else rs.push_back(polys[i].eval1(prng.get<oc::block>()));
    }
    t1.setTimePoint("end");

    std::cout << t0 << "\n" << t1 << "\n";

    if(DEBUG) {
        // for (int i = 0; i < n; ++i) if(rs[i] != values[i][ind[i]]) {
        //     std::cout << rs[i] << " " << values[i][ind[i]] << std::endl;
        //     throw std::runtime_error("!!!");
        // }
        for (int i = 0; i < n; ++i) for (int j = 0; j < keys[i].size(); ++j) {
            if(polys[i].eval1(keys[i][j]) != values[i][j]) {
                throw std::runtime_error("!!!");
            }
        }
        std::cout << "test passed\n\n";
    }
}

int main() {
    for (int i = 0; i < 10; ++i) one_run();
}