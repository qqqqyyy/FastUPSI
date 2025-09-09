#ifndef NETWORK_H
#define NETWORK_H

#include "ASE/ASE.h"

namespace upsi{

oc::cp::task<> send_ASE(const ASE& ase, oc::Socket* chl) {
    co_await chl->send(ase.n);
    // co_await chl->send(ase.elem_cnt);
    BlockVec data;
    ase.write(data);
    co_await chl->send(data);
    co_return;
}

oc::cp::task<ASE> recv_ASE(oc::Socket* chl) {
    size_t n;
    co_await chl->recv(n);
    ASE ase(n, true);
    // co_await chl->recv(ase.elem_cnt);
    BlockVec data;
    co_await chl->recvResize(data);
    ase.read(data);
    co_return ase;
}

oc::cp::task<> send_OPRF(const OPRFValueVec& values, oc::Socket* chl) {
    BlockVec tmp;
    for (auto x: values) {
        tmp.push_back(x.first);
        tmp.push_back(x.second);
    }
    co_await chl->send(tmp);
    co_return;
}

oc::cp::task<OPRFValueVec> recv_OPRF(oc::Socket* chl) {
    BlockVec tmp;
    co_await chl->recvResize(tmp);
    int cnt = tmp.size() >> 1;
    OPRFValueVec rs;
    rs.reserve(cnt);
    for (int i = 0; i < cnt; ++i) rs.push_back(std::make_pair(tmp[i << 1], tmp[i << 1 | 1]));
    co_return rs;
}

} // namespace upsi

#endif