// #pragma once

#include "libOTe/Vole/Silent/SilentVoleSender.h"
#include "libOTe/Vole/Silent/SilentVoleReceiver.h"

// #include "libOTe/Tools/CoeffCtx.h"
// #include <thread>
using namespace oc;
// using VecF = typename CoeffCtxGF128::template Vec<block>;
CoeffCtxGF128 ctx;
// const bool DEBUG = false;

// void test(VecF a, VecF b, VecF c, block delta) {
//     std::cout << "test VOLE correlations ...\n";
//     for (u64 i = 0; i < n; ++i) {
//         block exp;
//         ctx.mul(exp, delta, c[i]);
//         ctx.plus(exp, exp, b[i]);
//         if(a[i] != exp) {
//             std::cout << "i = " << i << std::endl;
//             throw std::runtime_error("Incorrect VOLE");
//         }
//     }
//     std::cout << "test ok\n";
// }

void doit() {
    u64 n = (1 << 14);
    Timer timer;
    timer.setTimePoint("start");
    SilentVoleSender<block, block, CoeffCtxGF128> sender;
    SilentVoleReceiver<block, block, CoeffCtxGF128> receiver;

    sender.configure(n, SilentSecType::SemiHonest, DefaultMultType, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);
    receiver.configure(n, SilentSecType::SemiHonest, DefaultMultType, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);

    // sender.mMultType = MultType::ExConv7x24;
    // receiver.mMultType = MultType::ExConv7x24;

    std::cout << "n = " << n << std::endl;
    
    PRNG prng0(ZeroBlock), prng1(ZeroBlock);
    // VecF a(n), b(n), c(n);
    block delta = prng0.get();

    // std::cout << delta << std::endl;

    auto chls = cp::LocalAsyncSocket::makePair();

            macoro::thread_pool pool0, pool1;
			auto w0 = pool0.make_work();
			auto w1 = pool1.make_work();
			pool0.create_thread();
			pool1.create_thread();
			chls[0].setExecutor(pool0);
			chls[1].setExecutor(pool1);
    
    auto t0 = std::thread([&] {
    for (int q = 0; q < (1 << 10); ++q) {
        if(q) {
            auto count = sender.baseCount();
            int t = count.mBaseVoleCount;
            span<block> subB(sender.mB.subspan(0, t));
            sender.setBaseCors({}, subB);
        }

        // timer.setTimePoint("s A");
        auto p0 = sender.silentSendInplace(delta, n, prng0, chls[0]) | macoro::start_on(pool0);
        coproto::sync_wait(p0);
        // timer.setTimePoint("s B");
    }
	});

    for (int q = 0; q < (1 << 10); ++q) {
        if(q) {
            auto count = receiver.baseCount();
            int t = count.mBaseVoleCount;
            span<block> subA(receiver.mA.subspan(0, t));
            span<block> subC(receiver.mC.subspan(0, t));
            receiver.setBaseCors({}, {}, subA, subC);
        }

        // timer.setTimePoint("r A");
        auto p1 = receiver.silentReceiveInplace(n, prng1, chls[1])| macoro::start_on(pool0);
        coproto::sync_wait(p1);
        // timer.setTimePoint("r B");
    }
    t0.join();

    timer.setTimePoint("end");
    std::cout << timer << std::endl;
}

int main() {
    doit();

    return 0;
}