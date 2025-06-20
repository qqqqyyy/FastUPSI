#include "libOTe/VOLE/Silent/SilentVoleReceiver.h"
#include "coproto/Socket/AsioSocket.h"
#include <chrono>

using namespace oc;

const u64 k = 1 << 14;
const u64 n = k + 128;
const u64 q = 1 << 10;
using VecF = typename CoeffCtxGF128::template Vec<block>;
CoeffCtxGF128 ctx;

task<> receive() {
    auto chl = cp::asioConnect("localhost:5001", false);

    SilentVoleReceiver<block, block, CoeffCtxGF128> receiver;
    receiver.configure(n, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);
    // receiver.mMultType = MultType::ExConv7x24;

    auto start = std::chrono::high_resolution_clock::now();

    PRNG prng(CCBlock);
    VecF a(n), c(n);

    co_await receiver.silentReceive(c, a, prng, chl);

    VecF subA, subC;
    for (u64 i = 1; i < q; ++i) {
        auto count = receiver.baseCount();
        int t = count.mBaseVoleCount;
        subA.resize(t);
        subC.resize(t);
        ctx.copy(a.begin(), a.begin() + t, subA.begin());
        ctx.copy(c.begin(), c.begin() + t, subC.begin());
        BitVector choices = receiver.sampleBaseChoiceBits(prng);
        std::vector<block> msg;
	    receiver.setBaseCors(choices, msg, subA, subC);
        if(!receiver.hasBaseCors()) throw std::runtime_error("base correlations not set");
        auto p = receiver.silentReceive(c, a, prng, chl);
        // co_await chl.flush();
    }

    co_await chl.flush();
    co_await chl.close();

    auto end = std::chrono::high_resolution_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time elapsed: " << duration.count() << " ms" << std::endl;
}

int main() {

    cp::sync_wait(receive());
    return 0;
}