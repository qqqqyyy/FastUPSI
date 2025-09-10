#include "tree_party.h"

namespace upsi{

std::vector<Element> TreeParty::query(const std::vector<Element>& elems) {
    int cnt = elems.size();
    OPRFValueVec values;
    for (int i = 0; i < cnt; ++i)
        other_tree.eval_oprf(elems[i], vole_sender.delta, ro_seed, values);
    random_shuffle<OPRFValue>(values);
    oc::cp::sync_wait(send_OPRF(values, chl));
    oc::cp::sync_wait(chl->flush());

    OPRFValueVec other_values = oc::cp::sync_wait(recv_OPRF(chl));
    std::vector<Element> rs;
    for (auto& cur_value: other_values) {
        auto tmp = oprf_data.find(cur_value);
        if(tmp.first) rs.push_back(tmp.second);
    }
    return rs;
}


void TreeParty::addition(const std::vector<Element>& elems) {
    
    oc::Timer t0("addition");
    t0.setTimePoint("begin");

    auto ins = my_tree.insert(elems);
    auto nodes = ins.first;
    auto ind = ins.second;
    int cnt = nodes.size();

    t0.setTimePoint("tree insert");


    // std::cout << "[other_addition] update...\n";

    size_t other_new_elem_cnt;
    oc::cp::sync_wait(chl->send(elems.size()));
    oc::cp::sync_wait(chl->recv(other_new_elem_cnt));


    std::vector<int> other_ind = other_tree.update(other_new_elem_cnt);
    int other_cnt = other_ind.size();

    // std::cout << cnt << " " << other_cnt << "\n";

    t0.setTimePoint("other party's tree insert");


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


    // std::cout << "[my_addition] stash...\n";

    rb_okvs stash(rb_okvs_size_table::get(DEFAULT_STASH_SIZE));

    std::vector<Element> tmp;
    my_tree.stash.getElements(tmp);
    stash.build(tmp, ro_seed);

    t0.setTimePoint("okvs encode");


    // std::cout << "[my_addition] polys oprf...\n";
    // std::cout << cnt << "\n";

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
    t0.setTimePoint("my polynomial oprf");


    // std::cout << "[my_addition] send, recv vole diff...\n";
    
    auto diffs = oc::cp::sync_wait(send_recv_ASEs(polys, chl));

    // std::vector<ASE> diffs;
    // if(party == 0) {
    //     oc::cp::sync_wait(send_ASEs(polys, chl));
    //     diffs = oc::cp::sync_wait(recv_ASEs(chl));
    // }
    // else {
    //     diffs = oc::cp::sync_wait(recv_ASEs(chl));
    //     oc::cp::sync_wait(send_ASEs(polys, chl));
    // }

    t0.setTimePoint("send/recv vole diff");

    // std::cout << "[other_addition] polys oprf...\n";

    // std::cout << diffs.size() << " " << other_cnt << "\n";

    for (int i = 0; i < other_cnt; ++i) {
        // ASE diff = oc::cp::sync_wait(recv_ASE(chl));
        diffs[i] *= vole_sender.delta;
        // if(diffs[i].n != 4) {
        //     std::cout << i << " " << diffs[i].n << "\n";
        //     throw std::runtime_error("!!!");
        // }
        diffs[i] += vole_sender.get(diffs[i].n);
        other_tree.binary_tree.nodes[other_ind[i]]->copy(diffs[i]);
    }

    t0.setTimePoint("other's polynomial oprf");


    // std::cout << "[my_addition] stash oprf...\n";

    OPRF<rb_okvs> oprf_okvs;
    auto vole = vole_receiver.get(stash.n);
    ASE c = stash - vole.second;
    rb_okvs a = rb_okvs(std::move(vole.first));
    a.setup(ro_seed);
    OPRFValueVec oprf_values;
    oprf_okvs.receiver(tmp, 0, a, oprf_values, ro_seed); //TODO for deletion
    oprf_data.remove(tmp);
    oprf_data.insert(tmp, oprf_values);

    oc::cp::sync_wait(send_ASE(c, chl));
    oc::cp::sync_wait(chl->flush());

    t0.setTimePoint("my okvs oprf");

    // std::cout << "[other_addition] stash oprf...\n";

    //stash
    ASE diff = oc::cp::sync_wait(recv_ASE(chl));
    diff *= vole_sender.delta;
    diff += vole_sender.get(diff.n);
    other_tree.stash.copy(diff);
    other_tree.stash.setup(ro_seed);

    t0.setTimePoint("other's okvs oprf");

    std::cout << t0 << "\n";
}



} // namespace upsi