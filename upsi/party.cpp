#include"party.h"

namespace upsi{

Party::Party(int _party, oc::Socket* _chl, int _total_days, std::string fn, bool deletion, bool daily_vole) {

    this->party = _party;
    this->chl = _chl;
    this->total_days = _total_days;
    this->support_deletion = deletion;
    this->daily_vole = daily_vole;

    dataset.Read(fn);

    if(dataset.start_size <= 64) dataset.print();
    
    if(total_days > dataset.days) throw std::runtime_error("dataset days error");
    if(support_deletion != (dataset.del_size > 0)) throw std::runtime_error("dataset number of deletions error");
    this->max_data_size = dataset.start_size + dataset.add_size * total_days;
    
    my_prng.SetSeed(oc::sysRandomSeed());
    vole_sender.setup(chl, &my_prng);
    vole_receiver.setup(chl, &my_prng);

    if(support_deletion) {
        oc::block del_seed;
        if(party == 0) {
            del_seed = oc::sysRandomSeed();
            coproto::sync_wait(chl->send(del_seed));
            coproto::sync_wait(chl->flush());
        }
        else coproto::sync_wait(chl->recv(del_seed));
        prng_del.SetSeed(del_seed);
    }
    
    oc::u8 tmp = 0;
    oc::cp::sync_wait(chl->send(tmp));
    oc::cp::sync_wait(chl->flush());
    oc::cp::sync_wait(chl->recv(tmp));

    if(!daily_vole) {
        size_t max_vole_size = (1 << oc::log2ceil(max_data_size)) * 4 + rb_okvs_size_table::get(DEFAULT_STASH_SIZE);
        max_vole_size += total_days * ( dataset.add_size * (4 * (oc::log2ceil(max_data_size) + 1)) + rb_okvs_size_table::get(DEFAULT_STASH_SIZE) );
        if(support_deletion) max_vole_size += total_days * (dataset.del_size * 4 + rb_okvs_size_table::get(DEFAULT_STASH_SIZE) + 1) + total_days * (1 << oc::log2ceil(max_data_size)) * 4 * 2;
        
        size_t last_comm = chl->bytesSent() + chl->bytesReceived();
        std::cout << "[VOLE] generate " << max_vole_size << " ...\n";
        oc::Timer t1("vole");
        t1.setTimePoint("vole begin");
        if(party == 0) {
            vole_sender.generate(max_vole_size);
            vole_receiver.generate(max_vole_size);
        }
        else {
            vole_receiver.generate(max_vole_size);
            vole_sender.generate(max_vole_size);
        }    
        t1.setTimePoint("vole generation end");
        std::cout << t1 << "\n";
        size_t cur_comm = chl->bytesSent() + chl->bytesReceived() - last_comm;
        std::cout << "[VOLE] Comm.(both parties) = " << cur_comm / 1024.0 / 1024.0 << " MB\n";
        std::cout << "[VOLE] done.\n\n";
    }
}

int Party::addition_part(const std::vector<Element>& addition_set) {

    std::vector<Element> I_plus;

    // oc::Timer t0("addition part");
    // t0.setTimePoint("begin");

    if(party == 0) {
        I_plus = query(addition_set);
        // t0.setTimePoint("query");
        auto I_1 = PSI_receiver(addition_set);
        // t0.setTimePoint("one-sided plain psi");
        I_plus.reserve(I_plus.size() + I_1.size());
        I_plus.insert(I_plus.end(), I_1.begin(), I_1.end());
        random_shuffle<Element>(I_plus);
        oc::cp::sync_wait(send_blocks(I_plus, chl));
        oc::cp::sync_wait(chl->flush());
        // t0.setTimePoint("I_plus");
    }
    else {
        auto cur_set = query(addition_set);
        // t0.setTimePoint("query");
        merge_set(cur_set, addition_set);
        PSI_sender(cur_set);
        // t0.setTimePoint("one-sided plain psi");
        I_plus = oc::cp::sync_wait(recv_blocks(chl));
        // t0.setTimePoint("I_plus");

    }
    
    addition(addition_set);
    // t0.setTimePoint("update");

    // intersection.reserve(intersection.size() + I_plus.size());
    // intersection.insert(intersection.end(), I_plus.begin(), I_plus.end());
    for (const auto& cur_elem: I_plus) intersection[cur_elem] = true;

    // t0.setTimePoint("end");
    // std::cout << t0 << "\n";

    return I_plus.size();
}

int Party::deletion_part(const std::vector<Element>& deletion_set) {

    // std::cout << "[deletion part] deletion ...\n";

    deletion(deletion_set);

    std::vector<Element> I_minus;

    if(party == 0) {
        // std::cout << "[deletion part] query ...\n";
        I_minus = query(std::vector<Element>());

        // std::cout << "[deletion part] I_minus ...\n";

        for (const auto& cur_elem: deletion_set) {
            if(intersection[cur_elem]) I_minus.push_back(cur_elem);
        }
        random_shuffle(I_minus);

        oc::cp::sync_wait(send_blocks(I_minus, chl));
    }
    else {
        // std::cout << "[deletion part] query ...\n";
        query(deletion_set);

        // std::cout << "[deletion part] I_minus ...\n";
        I_minus = oc::cp::sync_wait(recv_blocks(chl));
    }

    reset_all();
    refresh_oprfs();

    for (const auto& cur_elem: I_minus) intersection[cur_elem] = false;

    return I_minus.size();
}


std::vector<Element> Party::PSI_receiver(const std::vector<Element>& my_set) {
    int cnt = my_set.size();
    rb_okvs okvs(rb_okvs_size_table::get(cnt));
    okvs.build(my_set, ro_seed);

    if(daily_vole) {
        oc::cp::sync_wait(chl->send(okvs.n));
        size_t my_vole_size = rb_okvs_size_table::get(cnt);
        oc::Timer t_vole("PSI vole");
        t_vole.setTimePoint("begin");
        vole_receiver.generate(my_vole_size);
        cur_vole_size += my_vole_size;
        t_vole.setTimePoint("PSI vole");
        if(total_days <= 8) std::cout << t_vole << "\n";
    }

    auto vole = vole_receiver.get(okvs.n);
    ASE c = okvs - vole.second;
    oc::cp::sync_wait(send_ASE(c, chl));
    rb_okvs a = rb_okvs(std::move(vole.first));
    a.setup(ro_seed);
    OPRFValueVec oprf_values;
    OPRF<rb_okvs> oprf_okvs;
    oprf_okvs.receiver(my_set, 0, a, oprf_values, ro_seed);

    OPRFValueVec other_oprf_values = oc::cp::sync_wait(recv_OPRF(chl));

    return look_up(my_set, oprf_values, other_oprf_values);
}

void Party::PSI_sender(const std::vector<Element>& my_set) {

    if(daily_vole) {
        size_t other_vole_size;
        oc::cp::sync_wait(chl->recv(other_vole_size));
        oc::Timer t_vole("PSI vole");
        t_vole.setTimePoint("begin");
        vole_sender.generate(other_vole_size);
        t_vole.setTimePoint("PSI vole");
        if(total_days <= 8) std::cout << t_vole << "\n";
    }

    ASE diff = oc::cp::sync_wait(recv_ASE(chl));
    diff *= vole_sender.delta;
    diff += vole_sender.get(diff.n);
    rb_okvs b = rb_okvs(std::move(diff));
    b.setup(ro_seed);
    
    OPRFValueVec oprf_values;
    OPRF<rb_okvs> oprf_okvs;
    oprf_okvs.sender(my_set, 0, b, vole_sender.delta, oprf_values, ro_seed);
    
    std::mt19937_64 rng{std::random_device{}()};
    std::shuffle(oprf_values.begin(), oprf_values.end(), rng);
    oc::cp::sync_wait(send_OPRF(oprf_values, chl));
}



} // namespace upsi