#include "libOTe/VOLE/Silent/SilentVoleSender.h"
#include "libOTe/VOLE/Silent/SilentVoleReceiver.h"
// #include "libOTe/Tools/CoeffCtx.h"
#include <thread>
using namespace oc;
u64 n = (1 << 18) + 128;
using VecF = typename CoeffCtxGF128::template Vec<block>;
CoeffCtxGF128 ctx;

void test(VecF a, VecF b, VecF c, block delta) {
    std::cout << "test VOLE correlations ...\n";
    for (u64 i = 0; i < n; ++i) {
        block exp;
        ctx.mul(exp, delta, c[i]);
        ctx.plus(exp, exp, b[i]);
        if(a[i] != exp) {
            std::cout << "i = " << i << std::endl;
            throw std::runtime_error("Incorrect VOLE");
        }
    }
    std::cout << "test ok\n";
}

task<> doit() {
    SilentVoleSender<block, block, CoeffCtxGF128> sender;
    SilentVoleReceiver<block, block, CoeffCtxGF128> receiver;

    sender.configure(n, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);
    receiver.configure(n, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);

    // sender.mMultType = MultType::ExConv7x24;
    // receiver.mMultType = MultType::ExConv7x24;
    
    PRNG prng(CCBlock);
    VecF a(n), b(n), c(n);
    block delta = prng.get();

    // std::cout << delta << std::endl;

    auto chls = cp::LocalAsyncSocket::makePair();

    std::cout << "VOLE begin ...\n";
    sender.silentSend(delta, b, prng, chls[1]);
    receiver.silentReceive(c, a, prng, chls[0]);
    std::cout << "done ...\n";

    test(a, b, c, delta);

    

    for (int qwq = 1; qwq <= 10; ++qwq) {
    
        auto count = sender.baseCount();
        int t = count.mBaseVoleCount;
        std::cout << "t = " << t << std::endl;

        std::cout << "BaseOTs = " << count.mBaseOtCount << std::endl;

        std::vector<std::array<block, 2>> msg2(count.mBaseOtCount);
        BitVector choices = receiver.sampleBaseChoiceBits(prng);
        std::vector<block> msg(choices.size());
        for (auto& m : msg2){
            m[0] = prng.get();
            m[1] = prng.get();
        }
        for (auto i : rng(msg.size()))
            msg[i] = msg2[i][choices[i]];
        VecF subA(t), subB(t), subC(t);
        ctx.copy(a.begin(), a.begin() + t, subA.begin());
        ctx.copy(b.begin(), b.begin() + t, subB.begin());
        ctx.copy(c.begin(), c.begin() + t, subC.begin());
        
        sender.setBaseCors(msg2, subB);
        receiver.setBaseCors(choices, msg, subA, subC);

        a = VecF(n); b = VecF(n); c = VecF(n);

        std::cout << "VOLE begin ...\n";
        sender.silentSend(delta, b, prng, chls[1]);
        receiver.silentReceive(c, a, prng, chls[0]);
        std::cout << "done ...\n";

        test(a, b, c, delta);
    }
}

int main() {
    cp::sync_wait(doit());

    return 0;
}