#ifndef NETWORK_H
#define NETWORK_H

#include "ASE/ASE.h"

namespace upsi{

oc::cp::task<> send_ASE(const ASE& ase, oc::Socket* chl) {
    co_await chl->send(ase.n);
    co_await chl->send(ase.elem_cnt);
    BlockVec data;
    ase.write(data);
    co_await chl->send(data);
    co_return;
}

oc::cp::task<ASE> recv_ASE(oc::Socket* chl) {
    size_t n;
    co_await chl->recv(n);
    ASE ase(n, true);
    co_await chl->recv(ase.elem_cnt);
    BlockVec data;
    co_await chl->recvResize(data);
    ase.read(data);
    co_return ase;
}

} // namespace upsi

#endif