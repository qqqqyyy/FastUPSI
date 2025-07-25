#include "vole.h"

namespace upsi{

using namespace oc;

void VoleSender::generate(size_t vole_size){
    SilentVoleSender<block, block, CoeffCtxGF128> sender;
    size_t q = (vole_size + m - 1) / m;
    sender.configure(n, SilentSecType::SemiHonest, DefaultMultType, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);
    // sender.mMultType = MultType::ExConv7x24;
    // sender.mMultType = MultType::BlkAcc3x8;
    
    b = VecF(m * q);
    delta = prng->get();
    VecF tmp(n);

    for (u64 i = 0; i < q; ++i) {
        if(i) {
            auto count = sender.baseCount();
            int t = count.mBaseVoleCount;
            span<block> subB(tmp.subspan(m, t));
            sender.setBaseCors({}, subB);
        }
        cp::sync_wait(sender.silentSend(delta, tmp, *prng, *chl));
        
        std::copy(tmp.begin(), tmp.begin() + m, b.begin() + m * i);
    }

    cp::sync_wait(chl->flush());
    cp::sync_wait(chl->close());

}

void VoleReceiver::generate(size_t vole_size) {
    SilentVoleReceiver<block, block, CoeffCtxGF128> receiver;
    size_t q = (vole_size + m - 1) / m;
    receiver.configure(n, SilentSecType::SemiHonest, DefaultMultType, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);

    a = VecF(m * q);
    c = VecF(m * q);
    VecF tmp_a(n), tmp_c(n);

    for (u64 i = 0; i < q; ++i) {
        if(i) {
            auto count = receiver.baseCount();
            int t = count.mBaseVoleCount;
            span<block> subA(tmp_a.subspan(m, t));
            span<block> subC(tmp_c.subspan(m, t));
            receiver.setBaseCors({}, {}, subA, subC);
            // if(!receiver.hasBaseCors()) throw std::runtime_error("base correlations not set");
        }

        cp::sync_wait(receiver.silentReceive(tmp_c, tmp_a, *prng, *chl));

        std::copy(tmp_a.begin(), tmp_a.begin() + m, a.begin() + m * i);
        std::copy(tmp_c.begin(), tmp_c.begin() + m, c.begin() + m * i);

    }

    cp::sync_wait(chl->flush());
    cp::sync_wait(chl->close());

}

}