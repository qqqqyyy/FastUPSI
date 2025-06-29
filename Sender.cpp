#include "libOTe/Vole/Silent/SilentVoleSender.h"
#include "coproto/Socket/AsioSocket.h"
#include <fstream>

using namespace oc;

const u64 k = 1 << 14;
const u64 n = k + 0;
const u64 q = 1 << 10;
using VecF = typename CoeffCtxGF128::template Vec<block>;
CoeffCtxGF128 ctx;

const bool DEBUG = false;


task<> send() {
    auto chl = cp::asioConnect("localhost:5001", true);

    SilentVoleSender<block, block, CoeffCtxGF128> sender;
    sender.configure(n, SilentSecType::SemiHonest, DefaultMultType, SilentBaseType::BaseExtend, SdNoiseDistribution::Stationary);
    // sender.mMultType = MultType::ExConv7x24;
    // sender.mMultType = MultType::BlkAcc3x8;

    auto start = std::chrono::high_resolution_clock::now();

    PRNG prng(CCBlock);
    VecF b(n);
    block delta = prng.get();
    co_await sender.silentSend(delta, b, prng, chl);

    std::ofstream file("sender.txt", std::ios::binary);
    if(DEBUG) {
        for (int j = 0; j < n; ++j)
            file.write(reinterpret_cast<const char*>(&b[j]), sizeof(block));
        file.write(reinterpret_cast<const char*>(&delta), sizeof(block));
    }
    // auto tmp = b[0];


    VecF subB;
        auto count = sender.baseCount();
        int t = count.mBaseVoleCount;
        subB.resize(t);
        ctx.copy(b.begin(), b.begin() + t, subB.begin());
        std::vector<std::array<block, 2>> msg2;

    for (u64 i = 1; i < q; ++i) {
        // auto count = sender.baseCount();
        // int t = count.mBaseVoleCount;
        // subB.resize(t);
        // ctx.copy(b.begin(), b.begin() + t, subB.begin());
        // std::vector<std::array<block, 2>> msg2;
        sender.setBaseCors(msg2, subB);
        if(!sender.hasBaseCors()) throw std::runtime_error("base correlations not set");

        // b.clear(); b.resize(n);
        // for (int j = 0; j < n; ++j) b[j] = block(2, 0);
        co_await sender.silentSend(delta, b, prng, chl);
        // co_await chl.flush();

        // if(b[0] == tmp) std::cerr << "!!!\n";

        if(DEBUG && i % 16 == 0) {
            for (int j = 0; j < n; ++j)
                file.write(reinterpret_cast<const char*>(&b[j]), sizeof(block));
            file.write(reinterpret_cast<const char*>(&delta), sizeof(block));
        }
    }

    co_await chl.flush();
    co_await chl.close();

    auto end = std::chrono::high_resolution_clock::now(); 
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time elapsed: " << duration.count() << " ms" << std::endl;

    // if(DEBUG) {
        std::cout << "NoiseVecSize: \t" << sender.mNoiseVecSize << std::endl;
        std::cout << "NoiseWeight: \t" << sender.mNumPartitions << std::endl;
        std::cout << "SizePer: \t" << sender.mSizePer << std::endl;
        // auto count = sender.baseCount();
        std::cout << "Base VOLEs: \t" << count.mBaseVoleCount << std::endl;
        std::cout << "Base OTs: \t" << count.mBaseOtCount << std::endl;
    // }

    // if(!sender.gen().hasBaseOts()) std::cout << "???\n";
}

int main() {

    cp::sync_wait(send());

    return 0;
}