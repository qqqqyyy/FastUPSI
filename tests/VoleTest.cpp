// #pragma once

#include "vole.cpp"
#include "cryptoTools/Common/CLP.h"
using namespace oc;
using namespace upsi;

void test_correctness(ASE a, ASE b, ASE c, block delta, int n) {
    CoeffCtxGF128 ctx;
    for (int i = 0; i < n; ++i) {
        block exp;
        ctx.mul(exp, delta, c.ase[i]);
        ctx.plus(exp, exp, b.ase[i]);
        if(a.ase[i] != exp) {
            std::cout << "i = " << i << std::endl;
            throw std::runtime_error("Incorrect VOLE");
        }
    }
}

void test_generate_new(VoleSender& vole_sender, VoleReceiver& vole_receiver, int _n = 0) {
    std::cout << "test VOLE correlations ...\n";

    int n = _n? _n : (1 << 15) + rand() % (1 << 15);
    Timer timer;
    timer.setTimePoint("start");

    auto t0 = std::thread([&] { vole_sender.generate(n); });
    vole_receiver.generate(n);
    t0.join();
    
    timer.setTimePoint("end");
    std::cout << "n = " << n << ":\n"<< timer << std::endl;

    auto tmp = vole_receiver.get(n);
    test_correctness(tmp.first, vole_sender.get(n), tmp.second, vole_sender.delta, n);

    std::cout << "test ok\n\n";
}

void test_doerner_shelat(VoleSender& vole_sender, VoleReceiver& vole_receiver) {
    std::cout << "test Doerner-Shelat\n";

    int n = 20, point_cnt = 100;
    PRNG prng(CCBlock);
    int domain_size = (1 << (n - 1)) + prng.get<u64>() % (1 << (n - 1));

    auto t0 = std::thread([&] { vole_sender.generate(1 << 20); });
    vole_receiver.generate(1 << 20);
    t0.join();

    std::vector<size_t> points;
    for (int i = 0; i < point_cnt; ++i) points.push_back(prng.get<u64>() % domain_size);

    BlockVec values;
    for (int i = 0; i < point_cnt; ++i) values.push_back(toBlock(rand(), rand()));

    Timer timer;
    timer.setTimePoint("start");

    ASE x, y;
    t0 = std::thread([&] { x = vole_sender.generate(domain_size, point_cnt); });
    y = vole_receiver.generate(domain_size, values, points);
    t0.join();

    timer.setTimePoint("end");
    std::cout << timer << std::endl;

    
    // std::cout << points[0] << std::endl;
    int cnt = 0;
    for (int i = 0; i < domain_size; ++i) {
        if(x[i] != y[i]) ++cnt;
        // else std::cout << i << std::endl;
    }
    if(cnt != point_cnt) {
        std::cout << cnt << std::endl;
        throw std::runtime_error("Incorrect others");
    }


    CoeffCtxGF128 ctx;
    for (int i = 0; i < point_cnt; ++i) {
        int idx = points[i];
        block exp;
        ctx.mul(exp, vole_sender.delta, values[i]);
        ctx.plus(exp, exp, x[idx]);
        if(y[idx] != exp) {
            std::cout << "i = " << i << std::endl;
            std::cout << x[idx] << std::endl;
            std::cout << y[idx] << std::endl;
            throw std::runtime_error("Incorrect");
        }
    }

    std::cout << "test passed\n\n";
}

int main(int argc, char** argv) {
    oc::CLP clp(argc, argv);
    clp.setDefault("n", 0);
    int n = clp.get<int>("n");

    auto chl = cp::LocalAsyncSocket::makePair();
    PRNG prng0(ZeroBlock), prng1(OneBlock);

    VoleSender vole_sender;
    vole_sender.setup(&chl[0], &prng0);
    VoleReceiver vole_receiver;
    vole_receiver.setup(&chl[1], &prng1);

    if(n) {
        test_generate_new(vole_sender, vole_receiver, n);
    }
    else {
        for (int i = 0; i < 10; ++i) {
            test_generate_new(vole_sender, vole_receiver);
            test_doerner_shelat(vole_sender, vole_receiver);
        }
    }

    return 0;
}