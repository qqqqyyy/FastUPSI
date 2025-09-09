#include "tree_party.h"

namespace upsi{


void TreeParty::my_addition(std::vector<Element> elems) {
    
    chl->send(elems.size());

    auto ins = my_tree.insert(elems);
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

    OPRF<Poly> oprf_poly;
    for (int i = 0; i < cnt - 1; ++i) {
        auto vole = vole_receiver.get(polys[i].n);
        ASE c = polys[i] - vole.second;
        oc::cp::sync_wait(send_ASE(c, chl));
        Poly a = Poly(std::move(vole.first));
        OPRFValueVec oprf_values;
        oprf_poly.receiver(cur_elems[i], ind[i], a, oprf_values, ro_seed); //TODO for deletion
        oprf_data.remove(cur_elems[i]);
        oprf_data.insert(cur_elems[i], oprf_values);
    }

    rb_okvs stash(200); //TODO: okvs size

    std::vector<Element> tmp;
    nodes[cnt - 1]->getElements(tmp);
    stash.build(tmp, ro_seed);

    OPRF<rb_okvs> oprf_okvs;
    auto vole = vole_receiver.get(stash.n);
    ASE c = stash - vole.second;
    oc::cp::sync_wait(send_ASE(c, chl));
    rb_okvs a = rb_okvs(std::move(vole.first));
    OPRFValueVec oprf_values;
    oprf_okvs.receiver(cur_elems[cnt - 1], ind[cnt - 1], a, oprf_values, ro_seed); //TODO for deletion
    oprf_data.remove(cur_elems[cnt - 1]);
    oprf_data.insert(cur_elems[cnt - 1], oprf_values);
}

void TreeParty::other_addition() {
    size_t new_elem_cnt;

    chl->recv(new_elem_cnt);
}


} // namespace upsi