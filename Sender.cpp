#include "libOTe/Vole/Silent/SilentVoleSender.h"
#include "coproto/Socket/AsioSocket.h"
#include <fstream>

using namespace oc;

const u64 k = 1 << 14;
const u64 n = k;
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


    PRNG prng(CCBlock);
    VecF b(n);
    block delta = prng.get();

    std::ofstream file("sender.txt", std::ios::binary);
    if(DEBUG) {
        for (int j = 0; j < n; ++j)
            file.write(reinterpret_cast<const char*>(&b[j]), sizeof(block));
        file.write(reinterpret_cast<const char*>(&delta), sizeof(block));
    }

    Timer timer;
    timer.setTimePoint("start");

    for (u64 i = 0; i < q; ++i) {
        if(i) {
            auto count = sender.baseCount();
            int t = count.mBaseVoleCount;
            span<block> subB(b.subspan(0, t));
            sender.setBaseCors({}, subB);
            // if(!sender.hasBaseCors()) throw std::runtime_error("base correlations not set");
        }

        co_await sender.silentSend(delta, b, prng, chl);

        if(DEBUG && i % 16 == 0) {
            for (int j = 0; j < n; ++j)
                file.write(reinterpret_cast<const char*>(&b[j]), sizeof(block));
            file.write(reinterpret_cast<const char*>(&delta), sizeof(block));
        }
    }

    co_await chl.flush();
    co_await chl.close();

    timer.setTimePoint("end");
    std::cout << timer << std::endl;

    if(DEBUG) {
        std::cout << "NoiseVecSize: \t" << sender.mNoiseVecSize << std::endl;
        std::cout << "NoiseWeight: \t" << sender.mNumPartitions << std::endl;
        std::cout << "SizePer: \t" << sender.mSizePer << std::endl;
        // auto count = sender.baseCount();
        // std::cout << "Base VOLEs: \t" << count.mBaseVoleCount << std::endl;
        // std::cout << "Base OTs: \t" << count.mBaseOtCount << std::endl;
    }
}

int main() {

    cp::sync_wait(send());

    return 0;
}