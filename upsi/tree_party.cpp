#include "tree_party.h"

namespace upsi{

int TreeParty::addition_part(std::vector<Element> addition_set) {

    std::vector<Element> I_plus;

    if(party == 0) {
        sender(addition_set);
        I_plus = receiver();
        auto I_1 = PSI_receiver(addition_set);
        I_plus.reserve(I_plus.size() + I_1.size());
        I_plus.insert(I_plus.end(), I_1.begin(), I_1.end());
        random_shuffle<Element>(I_plus);
        oc::cp::sync_wait(chl->send(I_plus));
        oc::cp::sync_wait(chl->flush());

        my_addition(addition_set);
        other_addition();
    }
    else {
        auto cur_set = receiver();
        sender(addition_set);
        merge_set(cur_set, addition_set);
        PSI_sender(cur_set);
        oc::cp::sync_wait(chl->recvResize(I_plus));

        other_addition();
        my_addition(addition_set);
    }

    intersection.reserve(intersection.size() + I_plus.size());
    intersection.insert(intersection.end(), I_plus.begin(), I_plus.end());

    return I_plus.size();
}

int TreeParty::deletion_part(std::vector<Element> deletion_set) {
    //TODO
    return 0;
}

void TreeParty::sender(std::vector<Element> elems) {
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


void TreeParty::my_addition(std::vector<Element> elems) {
    
    chl->send(elems.size());

    auto ins = my_tree.insert(elems);
    auto nodes = ins.first;
    auto ind = ins.second;
    int cnt = nodes.size();

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

    OPRF<Poly> oprf_poly;
    for (int i = 0; i < cnt; ++i) {
        auto vole = vole_receiver.get(polys[i].n);
        ASE c = polys[i] - vole.second;
        oc::cp::sync_wait(send_ASE(c, chl));
        Poly a = Poly(std::move(vole.first));
        OPRFValueVec oprf_values;
        oprf_poly.receiver(cur_elems[i], ind[i], a, oprf_values, ro_seed); //TODO (for deletion)
        oprf_data.remove(cur_elems[i]);
        oprf_data.insert(cur_elems[i], oprf_values);
    }

    rb_okvs stash(DEFAULT_STASH_SIZE); //TODO: okvs size

    std::vector<Element> tmp;
    my_tree.stash.getElements(tmp);
    stash.build(tmp, ro_seed);

    OPRF<rb_okvs> oprf_okvs;
    auto vole = vole_receiver.get(stash.n);
    ASE c = stash - vole.second;
    oc::cp::sync_wait(send_ASE(c, chl));
    rb_okvs a = rb_okvs(std::move(vole.first));
    OPRFValueVec oprf_values;
    oprf_okvs.receiver(tmp, 0, a, oprf_values, ro_seed); //TODO for deletion
    oprf_data.remove(tmp);
    oprf_data.insert(tmp, oprf_values);
}

void TreeParty::other_addition() {
    size_t new_elem_cnt;

    chl->recv(new_elem_cnt);

    std::vector<int> ind = other_tree.update(new_elem_cnt);
    int cnt = ind.size();

    for (int i = 0; i < cnt; ++i) {
        ASE diff = oc::cp::sync_wait(recv_ASE(chl));
        diff *= vole_sender.delta;
        diff += vole_sender.get(diff.n);
        other_tree.binary_tree.nodes[ind[i]]->copy(diff);
    }

    //stash
    ASE diff = oc::cp::sync_wait(recv_ASE(chl));
    diff *= vole_sender.delta;
    diff += vole_sender.get(diff.n);
    other_tree.stash.copy(diff);
}


} // namespace upsi