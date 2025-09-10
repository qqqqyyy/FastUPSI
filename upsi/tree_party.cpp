#include "tree_party.h"

namespace upsi{

void TreeParty::sender(const std::vector<Element>& elems) {
    int cnt = elems.size();
    OPRFValueVec values;
    for (int i = 0; i < cnt; ++i)
        other_tree.eval_oprf(elems[i], vole_sender.delta, ro_seed, values);
    random_shuffle<OPRFValue>(values);
    oc::cp::sync_wait(send_OPRF(values, chl));
}

std::vector<Element> TreeParty::receiver() {
    OPRFValueVec values = oc::cp::sync_wait(recv_OPRF(chl));
    std::vector<Element> rs;
    for (auto& cur_value: values) {
        auto tmp = oprf_data.find(cur_value);
        if(tmp.first) rs.push_back(tmp.second);
    }
    return rs;
}


void TreeParty::my_addition(const std::vector<Element>& elems) {
    
    oc::Timer t0("my_addition");
    t0.setTimePoint("begin");
    oc::cp::sync_wait(chl->send(elems.size()));
    oc::cp::sync_wait(chl->flush());

    auto ins = my_tree.insert(elems);
    auto nodes = ins.first;
    auto ind = ins.second;
    int cnt = nodes.size();

    t0.setTimePoint("tree insert");

    // std::cout << "[my_addition] polys...\n";

    std::vector<Poly> polys(cnt);
    std::vector<BlockVec> cur_elems;
    std::vector<BlockVec> cur_values;
    for (int i = 0; i < cnt; ++i) {
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


    t0.setTimePoint("polynomial interpolation");


    // std::cout << "[my_addition] polys oprf...\n";
    std::cout << cnt << "\n";

    OPRF<Poly> oprf_poly;
    for (int i = 0; i < cnt; ++i) {
        auto vole = vole_receiver.get(polys[i].n);
        polys[i] -= vole.second;
        Poly a = Poly(std::move(vole.first));
        OPRFValueVec oprf_values;
        oprf_poly.receiver(cur_elems[i], ind[i], a, oprf_values, ro_seed); //TODO (for deletion)
        oprf_data.remove(cur_elems[i]);
        oprf_data.insert(cur_elems[i], oprf_values);
    }
    t0.setTimePoint("polynomial oprf");

    oc::cp::sync_wait(send_ASEs(polys, chl));
    oc::cp::sync_wait(chl->flush());


    t0.setTimePoint("sending vole diff");


    // std::cout << "[my_addition] stash...\n";

    rb_okvs stash(rb_okvs_size_table::get(DEFAULT_STASH_SIZE));

    std::vector<Element> tmp;
    my_tree.stash.getElements(tmp);
    stash.build(tmp, ro_seed);


    t0.setTimePoint("okvs encode");


    // std::cout << "[my_addition] stash oprf...\n";

    OPRF<rb_okvs> oprf_okvs;
    auto vole = vole_receiver.get(stash.n);
    ASE c = stash - vole.second;
    oc::cp::sync_wait(send_ASE(c, chl));
    rb_okvs a = rb_okvs(std::move(vole.first));
    a.setup(ro_seed);
    OPRFValueVec oprf_values;
    oprf_okvs.receiver(tmp, 0, a, oprf_values, ro_seed); //TODO for deletion
    oprf_data.remove(tmp);
    oprf_data.insert(tmp, oprf_values);

    oc::cp::sync_wait(chl->flush());

    t0.setTimePoint("okvs oprf");
    std::cout << t0 << "\n";
}

void TreeParty::other_addition() {
    size_t new_elem_cnt;

    oc::cp::sync_wait(chl->recv(new_elem_cnt));

    // std::cout << "[other_addition] update...\n";

    std::vector<int> ind = other_tree.update(new_elem_cnt);
    int cnt = ind.size();


    // std::cout << "[other_addition] polys oprf...\n";

    auto diffs = oc::cp::sync_wait(recv_ASEs(chl));

    for (int i = 0; i < cnt; ++i) {
        // ASE diff = oc::cp::sync_wait(recv_ASE(chl));
        diffs[i] *= vole_sender.delta;
        diffs[i] += vole_sender.get(diffs[i].n);
        other_tree.binary_tree.nodes[ind[i]]->copy(diffs[i]);
    }

    // std::cout << "[other_addition] stash oprf...\n";

    //stash
    ASE diff = oc::cp::sync_wait(recv_ASE(chl));
    diff *= vole_sender.delta;
    diff += vole_sender.get(diff.n);
    other_tree.stash.copy(diff);
    other_tree.stash.setup(ro_seed);
}


} // namespace upsi