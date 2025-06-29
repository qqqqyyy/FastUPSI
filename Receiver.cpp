#include "libOTe/Vole/Silent/SilentVoleReceiver.h"
#include "coproto/Socket/AsioSocket.h"
#include <chrono>
#include <fstream>

using namespace oc;

const u64 k = 1 << 14;
const u64 n = k + 0;
const u64 q = 1 << 10;
using VecF = typename CoeffCtxGF128::template Vec<block>;
CoeffCtxGF128 ctx;

const bool DEBUG = false;

void test(VecF a, VecF b, VecF c, block delta) {
    std::cout << "test VOLE correlations ...\n";
    for (u64 i = 0; i < n; ++i) {
        block exp;
        ctx.mul(exp, delta, c[i]);
        ctx.plus(exp, exp, b[i]);
        if(a[i] != exp) {
            std::cout << "i = " << i << std::endl;
            std::cout << a[i] << " " << b[i] << " " << c[i] << " " << delta << "\n";
            throw std::runtime_error("Incorrect VOLE");
        }
    }
    std::cout << "test ok\n";
}

task<> receive() {
    auto chl = cp::asioConnect("localhost:5001", false);

    SilentVoleReceiver<block, block, CoeffCtxGF128> receiver;
    receiver.configure(n, SilentSecType::SemiHonest, DefaultMultType, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);
    // receiver.mMultType = MultType::ExConv7x24;
    // receiver.mMultType = MultType::BlkAcc3x8;

    auto start = std::chrono::high_resolution_clock::now();

    PRNG prng(CCBlock);
    VecF a(n), c(n);

    co_await receiver.silentReceive(c, a, prng, chl);
        
    std::ofstream file("receiver.txt", std::ios::binary);
    if(DEBUG) {
        for (int j = 0; j < n; ++j)
            file.write(reinterpret_cast<const char*>(&a[j]), sizeof(block));
        for (int j = 0; j < n; ++j)
            file.write(reinterpret_cast<const char*>(&c[j]), sizeof(block));
    }

    VecF subA, subC;
        auto count = receiver.baseCount();
        int t = count.mBaseVoleCount;
        subA.resize(t);
        subC.resize(t);
        ctx.copy(a.begin(), a.begin() + t, subA.begin());
        ctx.copy(c.begin(), c.begin() + t, subC.begin());
        BitVector choices = receiver.sampleBaseChoiceBits(prng);
        std::vector<block> msg;

    for (u64 i = 1; i < q; ++i) {
        // auto count = receiver.baseCount();
        // int t = count.mBaseVoleCount;
        // subA.resize(t);
        // subC.resize(t);
        // ctx.copy(a.begin(), a.begin() + t, subA.begin());
        // ctx.copy(c.begin(), c.begin() + t, subC.begin());
        // BitVector choices = receiver.sampleBaseChoiceBits(prng);
        // std::vector<block> msg;
	    receiver.setBaseCors(choices, msg, subA, subC);
        if(!receiver.hasBaseCors()) throw std::runtime_error("base correlations not set");

        // for (int j = 0; j < n; ++j) a[j] = c[j] = block(0, 0);
        co_await receiver.silentReceive(c, a, prng, chl);
        // co_await chl.flush();
        if(DEBUG && i % 16 == 0) {
            for (int j = 0; j < n; ++j)
                file.write(reinterpret_cast<const char*>(&a[j]), sizeof(block));
            for (int j = 0; j < n; ++j)
                file.write(reinterpret_cast<const char*>(&c[j]), sizeof(block));
        }
    }

    co_await chl.flush();
    co_await chl.close();

    auto end = std::chrono::high_resolution_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time elapsed: " << duration.count() << " ms" << std::endl;

    if(DEBUG) {
        file.close();
        std::ifstream sender_file("sender.txt"), receiver_file("receiver.txt");
        for(int o = 1; o <= q/16; ++o) {
            VecF a(n), b(n), c(n); block delta;
            for (int j = 0; j < n; ++j)
                receiver_file.read(reinterpret_cast<char*>(&a[j]), sizeof(block));
            for (int j = 0; j < n; ++j)
                receiver_file.read(reinterpret_cast<char*>(&c[j]), sizeof(block));
            for (int j = 0; j < n; ++j)
                sender_file.read(reinterpret_cast<char*>(&b[j]), sizeof(block));
            sender_file.read(reinterpret_cast<char*>(&delta), sizeof(block));
            test(a, b, c, delta);
        }
    }
}

int main() {

    cp::sync_wait(receive());

    return 0;
}