#include "tree_party.h"

namespace upsi{

std::vector<Element> TreeParty::query(const std::vector<Element>& elems) {
    int cnt = elems.size();
    OPRFValueVec values;
    values.reserve(cnt * (other_tree.binary_tree.depth + 2));
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

void TreeParty::update_stash() {
    // std::cout << "[stash] update stash...\n";

    // oc::Timer t0("stash");
    // t0.setTimePoint("begin");

    rb_okvs stash(rb_okvs_size_table::get(DEFAULT_STASH_SIZE));

    std::vector<Element> tmp;
    my_tree.stash.getElements(tmp);
    stash.build(tmp, ro_seed);

    // t0.setTimePoint("okvs encode");


    // std::cout << "[stash] stash oprf...\n";

    OPRF<rb_okvs> oprf_okvs;
    auto vole = vole_receiver.get(stash.n);
    ASE c = stash - vole.second;
    rb_okvs a = rb_okvs(std::move(vole.first));
    a.setup(ro_seed);
    OPRFValueVec oprf_values;
    oprf_okvs.receiver(tmp, 0, a, oprf_values, ro_seed);
    if(support_deletion) my_tree_vole.stash = std::move(a);
    oprf_data.remove(tmp);
    oprf_data.insert(tmp, oprf_values);

    oc::cp::sync_wait(send_ASE(c, chl));
    oc::cp::sync_wait(chl->flush());

    // t0.setTimePoint("my okvs oprf");

    // std::cout << "[stash] other's stash oprf...\n";

    //stash
    ASE diff = oc::cp::sync_wait(recv_ASE(chl));
    diff *= vole_sender.delta;
    diff += vole_sender.get(diff.n);
    other_tree.stash.copy(diff);
    other_tree.stash.setup(ro_seed);

    // t0.setTimePoint("other's okvs oprf");
}


void TreeParty::addition(const std::vector<Element>& elems) {

    // std::cout << "[addition] begin ...\n";
    
    // oc::Timer t0("addition");
    // t0.setTimePoint("begin");

    auto ins = my_tree.insert(elems);
    auto nodes = ins.first;
    auto ind = ins.second;
    int cnt = nodes.size();

    // std::cout << "[addition] my tree vole insert ...\n";
    if(support_deletion) my_tree_vole.insert(elems);

    // t0.setTimePoint("tree insert");


    // std::cout << "[other_addition] update...\n";

    size_t other_new_elem_cnt;
    oc::cp::sync_wait(chl->send(elems.size()));
    oc::cp::sync_wait(chl->recv(other_new_elem_cnt));


    std::vector<int> other_ind = other_tree.update(other_new_elem_cnt);
    int other_cnt = other_ind.size();

    // std::cout << cnt << " " << other_cnt << "\n";

    // t0.setTimePoint("other party's tree insert");


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

    // t0.setTimePoint("polynomial interpolation");



    // std::cout << "[my_addition] polys oprf...\n";
    // std::cout << cnt << "\n";

    OPRF<Poly> oprf_poly;
    for (int i = 0; i < cnt; ++i) {
        auto vole = vole_receiver.get(polys[i].n);
        polys[i] -= vole.second;
        Poly a = Poly(std::move(vole.first));
        OPRFValueVec oprf_values;
        oprf_poly.receiver(cur_elems[i], ind[i], a, oprf_values, ro_seed);
        if(support_deletion) *(my_tree_vole.binary_tree.nodes[ind[i]]) = std::move(a);
        oprf_data.remove(cur_elems[i]);
        oprf_data.insert(cur_elems[i], oprf_values);
    }
    // t0.setTimePoint("my polynomial oprf");


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

    // t0.setTimePoint("send/recv vole diff");

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

    // t0.setTimePoint("other's polynomial oprf");


    update_stash();

    // t0.setTimePoint("stash");


    // std::cout << t0 << "\n";
}

void TreeParty::deletion(const std::vector<Element>& elems) {

    oc::Timer t0("deletion");
    t0.setTimePoint("begin");

    int cnt = elems.size();

    size_t other_del_elem_cnt;
    oc::cp::sync_wait(chl->send(elems.size()));
    oc::cp::sync_wait(chl->recv(other_del_elem_cnt));

    auto ind = my_tree.find(elems, true);
    std::sort(ind.begin(), ind.end());
    ind.erase(std::unique(ind.begin(), ind.end()), ind.end());

    // std::cout << "[deletion] polys...\n";
    t0.setTimePoint("find");

    std::vector<BlockVec> cur_elems;
    std::vector<BlockVec> cur_values;
    for (int index: ind) {
        if(index == 0) continue;
        std::vector<Element> tmp;
        BlockVec tmp_values;
        my_tree.binary_tree.nodes[index]->getElements(tmp);
        cur_elems.push_back(tmp);
        for (auto& x: tmp) {
            tmp_values.push_back(random_oracle(x, ro_seed));
        }
        cur_values.push_back(tmp_values);
    }
    std::vector<Poly> polys(cur_elems.size());
    batchInterpolation(polys, cur_elems, cur_values);

    t0.setTimePoint("polys");

    // std::cout << "[deletion] polys diff...\n";

    std::vector<size_t> points;
    BlockVec values;
    size_t ind_size = ind.size();
    for (int i = 0, j = 0; i < ind_size; ++i) {
        if(ind[i] == 0) continue;
        ASE diff = polys[j] - *(my_tree_vole.binary_tree.nodes[ind[i]]);
        for (int k = 0; k < DEFAULT_NODE_SIZE; ++k) {
            points.push_back(ind[i] * DEFAULT_NODE_SIZE + k);
            values.push_back(diff[k]);
        }
        my_tree_vole.binary_tree.nodes[ind[i]] = std::make_shared<Poly>(polys[j]);
        ++j;
    }
    int pad_cnt = DEFAULT_NODE_SIZE * cnt - points.size();
    for (int i = 0; i < pad_cnt; ++i) {
        points.push_back(0);
        values.push_back(oc::ZeroBlock);
    }

    t0.setTimePoint("polys diff");

    
    // std::cout << "[deletion] pprf...\n";
    // std::cout << my_tree_vole.binary_tree.n << " " << other_tree.binary_tree.n << "\n";

    if(party == 0) {
        ASE my_diff = vole_receiver.generate(my_tree_vole.binary_tree.n, values, points);
        my_tree_vole.binary_tree += my_diff;

        ASE other_diff = vole_sender.generate(other_tree.binary_tree.n, other_del_elem_cnt * DEFAULT_NODE_SIZE);
        other_tree.binary_tree += other_diff;
    }
    else {
        ASE other_diff = vole_sender.generate(other_tree.binary_tree.n, other_del_elem_cnt * DEFAULT_NODE_SIZE);
        other_tree.binary_tree += other_diff;

        ASE my_diff = vole_receiver.generate(my_tree_vole.binary_tree.n, values, points);
        my_tree_vole.binary_tree += my_diff;
    }

    t0.setTimePoint("pprf");


    update_stash();

    t0.setTimePoint("stash");


    // std::cout << "[deletion] update oprf values...\n";

    if(cnt == 0 || other_del_elem_cnt == 0) reset_all();


    for(const auto& cur_elem: elems) oprf_data.remove(cur_elem);

    refresh_oprfs();

    t0.setTimePoint("recompute oprf values");

    std::cout << t0 << "\n";


    // std::cout << "[deletion] done.\n\n";
}

void TreeParty::reset_all() {
    if(party == 0) {
        my_tree_vole.binary_tree += ASE(GetRandomSet(&prng_del, my_tree_vole.binary_tree.n));
        my_tree_vole.stash += ASE(GetRandomSet(&prng_del, my_tree_vole.stash.n));

        other_tree.binary_tree +=  ASE(GetRandomSet(&prng_del, other_tree.binary_tree.n));
        other_tree.stash += ASE(GetRandomSet(&prng_del, other_tree.stash.n));
    }
    else {
        other_tree.binary_tree +=  ASE(GetRandomSet(&prng_del, other_tree.binary_tree.n));
        other_tree.stash += ASE(GetRandomSet(&prng_del, other_tree.stash.n));

        my_tree_vole.binary_tree += ASE(GetRandomSet(&prng_del, my_tree_vole.binary_tree.n));
        my_tree_vole.stash += ASE(GetRandomSet(&prng_del, my_tree_vole.stash.n));
    }
}

void TreeParty::refresh_oprfs() {
    oprf_data.clear();
    
    OPRFValueVec values;
    OPRF<rb_okvs> oprf_okvs;
    OPRF<Poly> oprf_poly;

    std::vector<Element> elems;
    elems.reserve(dataset.start_size + (dataset.add_size - dataset.del_size) * current_day);

    std::vector<Element> cur_elems;
    my_tree.stash.getElements(cur_elems);
    elems.insert(elems.end(), cur_elems.begin(), cur_elems.end());
    oprf_okvs.receiver(cur_elems, 0, my_tree_vole.stash, values, ro_seed);
    cur_elems.clear();

    int cnt = my_tree.binary_tree.nodes.size();

    for (int i = 1; i < cnt; ++i) {
        my_tree.binary_tree.nodes[i]->getElements(cur_elems);
        if(cur_elems.size() > 0) {
            elems.insert(elems.end(), cur_elems.begin(), cur_elems.end());
            oprf_poly.receiver(cur_elems, i, *(my_tree_vole.binary_tree.nodes[i]), values, ro_seed);
            cur_elems.clear();
        }
    }
    oprf_data.insert(elems, values);
}



} // namespace upsi