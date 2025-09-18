#include "vole.h"

namespace upsi{

using namespace oc;

void VoleSender::generate(size_t vole_size){
    // compute_parameters(vole_size);
    size_t q = (vole_size + m - 1) / m;
    // sender.configure(n, SilentSecType::SemiHonest, DefaultMultType, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);
    // sender.mMultType = MultType::ExConv7x24;
    // sender.mMultType = MultType::BlkAcc3x8;
    
    b = VecF(m * q);
    VecF tmp(n);

    for (size_t i = 0; i < q; ++i) {
        if(has_base) {
            auto count = sender.baseCount();
            int t = count.mBaseVoleCount;
            if(count.mBaseOtCount > 0) {
                std::cout << i << " " << count.mBaseOtCount << "!!!\n";
            }
            sender.setBaseCors({}, base_b.subspan(0, t));
        }
        cp::sync_wait(sender.silentSend(delta, tmp, *prng, *chl));
        
        std::copy(tmp.begin(), tmp.begin() + m, b.begin() + m * i);
        std::copy(tmp.begin() + m, tmp.begin() + m + 128, base_b.begin());
        has_base = true;
    }

    cp::sync_wait(chl->flush());

    idx = 0;

}

void VoleReceiver::generate(size_t vole_size) {
    // compute_parameters(vole_size);
    size_t q = (vole_size + m - 1) / m;
    // receiver.configure(n, SilentSecType::SemiHonest, DefaultMultType, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);

    a = VecF(m * q);
    c = VecF(m * q);
    VecF tmp_a(n), tmp_c(n);

    for (size_t i = 0; i < q; ++i) {
        if(has_base) {
            auto count = receiver.baseCount();
            int t = count.mBaseVoleCount;
            receiver.setBaseCors({}, {}, base_a.subspan(0, t), base_c.subspan(0, t));
            // if(!receiver.hasBaseCors()) throw std::runtime_error("base correlations not set");
        }

        cp::sync_wait(receiver.silentReceive(tmp_c, tmp_a, *prng, *chl));

        std::copy(tmp_a.begin(), tmp_a.begin() + m, a.begin() + m * i);
        std::copy(tmp_c.begin(), tmp_c.begin() + m, c.begin() + m * i);

        std::copy(tmp_a.begin() + m, tmp_a.begin() + m + 128, base_a.begin());
        std::copy(tmp_c.begin() + m, tmp_c.begin() + m + 128, base_c.begin());
        has_base = true;

    }

    cp::sync_wait(chl->flush());

    idx = 0;

}

/////////////////doerner-shelat

ASE VoleSender::generate(int domain_size, int point_cnt) {

    // oc::Timer t_vole("doerner-shelat");
    // t_vole.setTimePoint("begin");

    int tmp_domain_size = domain_size;
    if(tmp_domain_size & 1) ++tmp_domain_size;

    // generate secret shares of delta * v
    ASE cur_b = get(point_cnt);
    // std::cout << "sender " << data.size() << std::endl;
    ASE tmp = cp::sync_wait(recv_ASE(chl));
    cur_b += (tmp *= delta);


    // t_vole.setTimePoint("diff");

    // std::cerr << "sender secret shares" << std::endl;

    // base OTs for PPRF
    pprf_sender.configure(tmp_domain_size, point_cnt);

    auto numOTs = pprf_sender.baseOtCount();
    ot_sender.configure(
        numOTs, 
        2, 
        1, // number of threads
        SilentSecType::SemiHonest, 
        SdNoiseDistribution::Stationary,
        DefaultMultType
    );
    std::vector<std::array<block,2>> rot(numOTs);
    coproto::sync_wait(ot_sender.send(rot, *prng, *chl));


    // t_vole.setTimePoint("ot");

    pprf_sender.setBase(rot);
    // for (int i = 0; i < pprf_sender.mDepth; ++i) std::cout << rot[i][0] << " " << rot[i][1] << std::endl;

    // std::cerr << "base OTs" << std::endl;

    // expand
    ASE rs(domain_size, true);
    pprf_sender.mOutputFn = [&](u64 treeIdx, VecF& leafs){
        // std::cout << domain_size << " " << leafs.size() << std::endl;
        // if(treeIdx > point_cnt || leafs.size() > tmp_domain_size) throw std::runtime_error("!!!");
        for (uint o = 0; o < 8 && o < point_cnt - treeIdx; ++o) {
            for (int i = 0; i < domain_size; ++i) rs[i] ^= leafs[(i << 3) + o];
        }
    };
    VecF dummy;
    coproto::sync_wait(pprf_sender.expand(
        *chl, 
        cur_b.VecF(), 
        prng->get(), 
        dummy, // output ignored
        oc::PprfOutputFormat::Callback, 
        true, // program
        1, // number of threads
        CoeffCtxGF128{}
    ));

    // t_vole.setTimePoint("pprf");
    // std::cout << t_vole << "\n";

    return rs;
}

ASE VoleReceiver::generate(int domain_size, BlockVec values, std::vector<size_t> points){

    // oc::Timer t_vole("doerner-shelat");
    // t_vole.setTimePoint("begin");
    
    int tmp_domain_size = domain_size;
    if(tmp_domain_size & 1) ++tmp_domain_size;
    int point_cnt = points.size();

    // generate secret shares of delta * v
    auto base_vole = get(point_cnt);
    ASE diff = ASE(values) - base_vole.second;
    // std::cout << "receiver " << data.size() << std::endl;
    coproto::sync_wait(send_ASE(diff, chl));
    coproto::sync_wait(chl->flush());

    // t_vole.setTimePoint("diff");

    // std::cerr << "receiver secret shares" << std::endl;

    // base OTs for PPRF
    pprf_receiver.configure(tmp_domain_size, point_cnt);

    auto numOTs = pprf_receiver.baseOtCount();
    BitVector recvBits = Points2Bits(points, pprf_receiver.mDepth);
    pprf_receiver.setChoiceBits(recvBits);
    ot_receiver.configure(
        numOTs, 
        2, 
        1, // number of threads
        SilentSecType::SemiHonest, 
        SdNoiseDistribution::Stationary,
        DefaultMultType
    );
    std::vector<block> rot_c(numOTs);
    coproto::sync_wait(ot_receiver.receive(recvBits, rot_c, *prng, *chl));


    // t_vole.setTimePoint("ot");

    // std::cout << pprf_receiver.mDomain << std::endl;
    // std::cout << points[0] << std::endl;
    // for(int i = 0; i < pprf_receiver.mDepth; ++i) std::cout << recvBits[i] << " " << rot_c[i] << std::endl;

    pprf_receiver.setBase(rot_c);

    // std::cerr << "base OTs" << std::endl;

    // expand
    ASE rs(domain_size, true);
    pprf_receiver.mOutputFn = [&](u64 treeIdx, VecF& leafs){
        // std::cout << domain_size << " " << leafs.size() << std::endl;
        // if(treeIdx > point_cnt || leafs.size() > tmp_domain_size) throw std::runtime_error("!!!");
        for (uint o = 0; o < 8 && o < point_cnt - treeIdx; ++o) {
            for (int i = 0; i < domain_size; ++i) rs[i] ^= leafs[(i << 3) + o];
        }
    };
    VecF dummy;
    coproto::sync_wait(pprf_receiver.expand(
        *chl, 
        dummy, // output ignored
        oc::PprfOutputFormat::Callback, 
        true, // program
        1, // number of threads
        CoeffCtxGF128{}
    )); 


    // t_vole.setTimePoint("pprf");

    for (int i = 0; i < point_cnt; ++i) rs[points[i]] ^= base_vole.first[i];


    // t_vole.setTimePoint("done");

    // std::cout << t_vole << "\n";

    return rs;
}

BitVector Points2Bits(const std::vector<size_t>& points, int depth) {
    int point_cnt = points.size();
    BitVector ans(point_cnt * depth);
    for (int i = 0; i < point_cnt; ++i) {
        size_t x = points[i];
        for (int j = 0; j < depth; ++j) ans[i * depth + j] = (x >> j) & 1;
    }
    return ans;
}

}