#include "tree_party.h"

namespace upsi{


void TreeParty::my_addition(std::vector<Element> elems) {
    auto nodes = my_tree.insert(elems, &tree_prng);
    int cnt = nodes.size();

    std::vector<Poly> polys(cnt - 1);
    std::vector<BlockVec> cur_elems;
    std::vector<BlockVec>  cur_values;
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

    rb_okvs stash(200); //TODO: size

    std::vector<Element> tmp;
    nodes[cnt - 1]->getElements(tmp);
    stash.build(tmp, ro_seed);

    OPRFValueVec values;
    OPRF<Poly> oprf_poly;
    for (int i = 0; i < cnt - 1; ++i) {
        auto vole = vole_receiver.get(polys[i].n);
        Poly a = Poly(std::move(vole.first));
        oprf_poly.receiver(cur_elems[i], a, values, ro_seed); //TODO
        ASE c = vole.second;
    } //TODO
}


} // namespace upsi