#include "adaptive_party.h"

namespace upsi{

template<typename BaseType>
std::vector<Element> AdaptiveParty<BaseType>::query(const std::vector<Element>& elems) {
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

template<typename BaseType>
void AdaptiveParty<BaseType>::addition(const std::vector<Element>& elems) {
    // oc::Timer t0("addition");
    // t0.setTimePoint("begin");

    BlockVec new_seeds;
    auto ins = my_adaptive.insert(elems, new_seeds);
    auto nodes = ins.first;
    auto ind = ins.second;
    int cnt = nodes.size();


    if(support_deletion) {
        auto ind_tmp = my_vole.update(elems.size());
        ind_tmp = my_adaptive_encrypted.update(elems.size());
    }

    // t0.setTimePoint("adaptive insert");

    // std::cout << "[other_addition] update...\n";

    size_t other_new_elem_cnt;
    oc::cp::sync_wait(chl->send(elems.size()));
    oc::cp::sync_wait(chl->recv(other_new_elem_cnt));
    // COMM += sizeof(size_t) * 2;


    std::vector<int> other_ind = other_adaptive.update(other_new_elem_cnt);
    int other_cnt = other_ind.size();

    // t0.setTimePoint("other party's adaptive insert");

    // std::cout << "[my_addition] nodes...\n";

    std::vector<BaseType> base_ASEs;
    std::vector<std::vector<Element> > cur_elems;
    for (int i = 0; i < cnt; ++i) {
        std::vector<Element> tmp;
        nodes[i]->getElements(tmp);
        cur_elems.push_back(tmp);
        BaseType cur_ASE(nodes[i]->n);
        cur_ASE.build(tmp, new_seeds[i]);
        base_ASEs.push_back(cur_ASE);
    }

    // t0.setTimePoint("ASE encode");

    if(daily_vole) {
        size_t my_vole_size = 0;
        size_t other_vole_size = 0;
        for(const auto& cur_ASE: base_ASEs) my_vole_size += cur_ASE.n; 
        for(int index: other_ind) other_vole_size += other_adaptive.nodes[index]->n;
        // oc::Timer t_vole("adaptive vole");
        // t_vole.setTimePoint("begin");
        if(party == 0) {
            vole_receiver.generate(my_vole_size);
            vole_sender.generate(other_vole_size);
        }
        else {
            vole_sender.generate(other_vole_size);
            vole_receiver.generate(my_vole_size);
        }
        cur_vole_size += my_vole_size;
        // t_vole.setTimePoint("adaptive vole");
        // if(total_days <= 8) std::cout << t_vole << "\n";
    }


    OPRF<BaseType> oprfs;

    for (int i = 0; i < cnt; ++i) {
        auto vole = vole_receiver.get(base_ASEs[i].n);
        if(support_deletion) 
            my_adaptive_encrypted.nodes[ind[i]] = std::make_shared<BaseType>(base_ASEs[i]);
        base_ASEs[i] -= vole.second;
        BaseType a = BaseType(std::move(vole.first));
        a.setup(new_seeds[i]);
        OPRFValueVec oprf_values;
        if constexpr (std::is_same_v<BaseType, rb_okvs>) oprfs.receiver(cur_elems[i], ind[i], a, oprf_values, new_seeds[i]);
        else if constexpr (std::is_same_v<BaseType, HashTable>) 
            oprfs.receiver_plain(cur_elems[i], ind[i], a, base_ASEs[i], oprf_values, new_seeds[i]);
        if(support_deletion) my_vole.nodes[ind[i]] = std::make_shared<BaseType>(std::move(a));
        oprf_data.remove(cur_elems[i]);
        oprf_data.insert(cur_elems[i], oprf_values);
    }

    // t0.setTimePoint("my ASE oprf");

    BlockVec other_new_seeds(other_cnt);
    oc::cp::sync_wait(chl->send(new_seeds));
    oc::cp::sync_wait(chl->recv(other_new_seeds));
    // COMM += (new_seeds.size() + other_new_seeds.size()) * sizeof(oc::block);
    auto diffs = oc::cp::sync_wait(send_recv_ASEs(base_ASEs, chl));

    // std::cout << cnt << " " << other_cnt << "\n";

    // t0.setTimePoint("send/recv vole diff");

    for (int i = 0; i < other_cnt; ++i) {
        diffs[i] *= vole_sender.delta;
        diffs[i] += vole_sender.get(diffs[i].n);
        other_adaptive.nodes[other_ind[i]]->copy(diffs[i]);
        other_adaptive.nodes[other_ind[i]]->setup(other_new_seeds[i]);
        other_adaptive.seeds[other_ind[i]] = other_new_seeds[i];
    }

    // t0.setTimePoint("other okvs oprf");

    // std::cout << t0 << "\n";
}

template<typename BaseType>
void AdaptiveParty<BaseType>::deletion(const std::vector<Element>& elems) {

    // oc::Timer t0("deletion");
    // t0.setTimePoint("begin");

    int cnt = elems.size();

    size_t other_del_elem_cnt;
    oc::cp::sync_wait(chl->send(elems.size()));
    oc::cp::sync_wait(chl->recv(other_del_elem_cnt));
    // COMM += sizeof(size_t) * 2;

    if(daily_vole) {
        size_t my_vole_size = my_vole.n;
        size_t other_vole_size = other_adaptive.n;
        // oc::Timer t_vole("deletion vole");
        // t_vole.setTimePoint("begin");
        if(party == 0) {
            vole_receiver.generate(my_vole_size);
            vole_sender.generate(other_vole_size);
        }
        else {
            vole_sender.generate(other_vole_size);
            vole_receiver.generate(my_vole_size);
        }
        cur_vole_size += my_vole_size;
        // t_vole.setTimePoint("deletion vole");
        // if(total_days <= 8) std::cout << t_vole << "\n";
    }

    auto pos = my_adaptive_encrypted.findPos2(elems, true); 
    auto ASE_ind = pos.first;
    auto points = pos.second;
    BlockVec values;
    //also remove them from plaintext ASE
    for(int i = 0; i < cnt; ++i) my_adaptive.nodes[ASE_ind[i]]->find(elems[i], true);
    
    //TODO: calculate the diffs
    for (int i = 0; i < cnt; ++i) {
        values.push_back(my_adaptive_encrypted[points[i]]);
        my_adaptive_encrypted[points[i]] = oc::ZeroBlock;
    }

    
    // std::cout << "[deletion] pprf...\n";
    // std::cout << my_tree_vole.binary_tree.n << " " << other_tree.binary_tree.n << "\n";

    if(party == 0) {
        ASE my_diff = vole_receiver.generate(my_vole.n, values, points);
        my_vole += my_diff;

        ASE other_diff = vole_sender.generate(other_adaptive.n, other_del_elem_cnt);
        other_adaptive += other_diff;
    }
    else {
        ASE other_diff = vole_sender.generate(other_adaptive.n, other_del_elem_cnt);
        other_adaptive += other_diff;

        ASE my_diff = vole_receiver.generate(my_vole.n, values, points);
        my_vole += my_diff;
    }

    // t0.setTimePoint("pprf");



    // std::cout << "[deletion] update oprf values...\n";

    if(cnt == 0 || other_del_elem_cnt == 0) reset_all();

    for(const auto& cur_elem: elems) oprf_data.remove(cur_elem);

    refresh_oprfs();

    // t0.setTimePoint("recompute oprf values");

    // std::cout << t0 << "\n";


    // std::cout << "[deletion] done.\n\n";
}

template<typename BaseType>
void AdaptiveParty<BaseType>::reset_all() {
    if(party == 0) {
        my_vole += ASE(GetRandomSet(&prng_del, my_vole.n));
        other_adaptive +=  ASE(GetRandomSet(&prng_del, other_adaptive.n));
    }
    else {
        other_adaptive +=  ASE(GetRandomSet(&prng_del, other_adaptive.n));
        my_vole += ASE(GetRandomSet(&prng_del, my_vole.n));
    }
}

template<typename BaseType>
void AdaptiveParty<BaseType>::refresh_oprfs() {
    oprf_data.clear();
    
    OPRF<BaseType> oprf;
    OPRFValueVec values;
    std::vector<Element> elems;
    elems.reserve(dataset.start_size + (dataset.add_size - dataset.del_size) * current_day);
    values.reserve(dataset.start_size + (dataset.add_size - dataset.del_size) * current_day);
    
    auto pos = my_adaptive_encrypted.findPos2(elems, false);
    auto ASE_ind = pos.first;
    auto points = pos.second;
    for (int i = 0; i < elems.size(); ++i)
        values.push_back(oprf.receiver_plain(elems[i], ASE_ind[i], my_vole[points[i]], my_adaptive.seeds[ASE_ind[i]]));

    oprf_data.insert(elems, values);
}

template class AdaptiveParty<rb_okvs>;
template class AdaptiveParty<HashTable>;

} // namespace upsi