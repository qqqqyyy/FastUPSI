#include "adaptive_party.h"

namespace upsi{

std::vector<Element> AdaptiveParty::query(const std::vector<Element>& elems) {
    int cnt = elems.size();
    OPRFValueVec values;
    for (int i = 0; i < cnt; ++i)
        other_adaptive.eval_oprf(elems[i], vole_sender.delta, values);
    random_shuffle<OPRFValue>(values);
    oc::cp::sync_wait(send_OPRF(values, chl));
    oc::cp::sync_wait(chl->flush());

    OPRFValueVec other_values = oc::cp::sync_wait(recv_OPRF(chl));
    std::vector<Element> rs;
    for (auto& cur_value: other_values) {
        auto tmp = oprf_data.find(cur_value);
        if(tmp.first) rs.push_back(tmp.second);
    }

    // std::cout << values.size() <<"\n";
    return rs;
}

void AdaptiveParty::addition(const std::vector<Element>& elems) {
    oc::Timer t0("addition");
    t0.setTimePoint("begin");

    BlockVec new_seeds;
    auto ins = my_adaptive.insert(elems, new_seeds);
    auto nodes = ins.first;
    auto ind = ins.second;
    int cnt = nodes.size();

    t0.setTimePoint("adaptive insert");


    // std::cout << "[other_addition] update...\n";

    size_t other_new_elem_cnt;
    oc::cp::sync_wait(chl->send(elems.size()));
    oc::cp::sync_wait(chl->recv(other_new_elem_cnt));


    std::vector<int> other_ind = other_adaptive.update(other_new_elem_cnt);
    int other_cnt = other_ind.size();


    t0.setTimePoint("other party's adaptive insert");


    // std::cout << "[my_addition] nodes...\n";

    std::vector<rb_okvs> okvs;
    std::vector<std::vector<Element> > cur_elems;
    for (int i = 0; i < cnt; ++i) {
        std::vector<Element> tmp;
        nodes[i]->getElements(tmp);
        cur_elems.push_back(tmp);
        rb_okvs cur_okvs(rb_okvs_size_table::get(nodes[i]->n));
        cur_okvs.build(tmp, new_seeds[i]);
        okvs.push_back(cur_okvs);
    }

    t0.setTimePoint("okvs encode");


    OPRF<rb_okvs> oprf_okvs;

    for (int i = 0; i < cnt; ++i) {
        auto vole = vole_receiver.get(okvs[i].n);
        okvs[i] -= vole.second;
        rb_okvs a = rb_okvs(std::move(vole.first));
        a.setup(new_seeds[i]);
        OPRFValueVec oprf_values;
        oprf_okvs.receiver(cur_elems[i], ind[i], a, oprf_values, new_seeds[i]); //TODO (for deletion)
        oprf_data.remove(cur_elems[i]);
        oprf_data.insert(cur_elems[i], oprf_values);
    }

    t0.setTimePoint("my okvs oprf");

    BlockVec other_new_seeds(other_cnt);
    oc::cp::sync_wait(chl->send(new_seeds));
    oc::cp::sync_wait(chl->recv(other_new_seeds));
    auto diffs = oc::cp::sync_wait(send_recv_ASEs(okvs, chl));

    // std::cout << cnt << " " << other_cnt << "\n";

    t0.setTimePoint("send/recv vole diff");

    for (int i = 0; i < other_cnt; ++i) {
        diffs[i] *= vole_sender.delta;
        diffs[i] += vole_sender.get(diffs[i].n);
        other_adaptive.nodes[other_ind[i]]->copy(diffs[i]);
        other_adaptive.nodes[other_ind[i]]->setup(other_new_seeds[i]);
        other_adaptive.seeds[other_ind[i]] = other_new_seeds[i];
    }

    t0.setTimePoint("other okvs oprf");

    std::cout << t0 << "\n";
}

} // namespace upsi