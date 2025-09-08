#include "tree_party.h"

namespace upsi{


void TreeParty::my_addition(std::vector<Element> elems) {
    auto ins = my_tree.insert(elems, &tree_prng);
    auto nodes = ins.first;
    auto ind = ins.second;
    int cnt = nodes.size();

    std::vector<Poly> polys(cnt - 1);
    std::vector<BlockVec> cur_elems;
    std::vector<BlockVec> cur_values;
    for (int i = 0; i < cnt - 1; ++i) {
        std::vector<Element> tmp;
        BlockVec tmp_values;
        nodes[i]->getElements(tmp);
        cur_elems.push_back(tmp);
        for (auto& x: tmp) {
            tmp_values.push_back(random_oracle(x, ro_seed));
        }
        cur_values.push_back(tmp_values);
    }
    batchInterpolation(polys, cur_elems, cur_values);

    OPRFValueVec values;
    OPRF<Poly> oprf_poly;
    for (int i = 0; i < cnt - 1; ++i) {
        auto vole = vole_receiver.get(polys[i].n);
        ASE c = polys[i] - vole.second;
        oc::cp::sync_wait(send_ASE(c, chl));
        Poly a = Poly(std::move(vole.first));
        oprf_poly.receiver(cur_elems[i], ind[i], a, values, ro_seed); //TODO
    } //TODO

    rb_okvs stash(200); //TODO: okvs size

    std::vector<Element> tmp;
    nodes[cnt - 1]->getElements(tmp);
    stash.build(tmp, ro_seed);

    
}


} // namespace upsi