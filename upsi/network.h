#ifndef NETWORK_H
#define NETWORK_H

#include "macoro/when_all.h"
#include "ASE/ASE.h"

namespace upsi{

inline oc::cp::task<> send_ASE(const ASE& ase, oc::Socket* chl) {
    co_await chl->send(ase.n);
    // co_await chl->send(ase.elem_cnt);
    BlockVec data;
    ase.write(data);
    co_await chl->send(data);
    co_return;
}

inline oc::cp::task<ASE> recv_ASE(oc::Socket* chl) {
    size_t n;
    co_await chl->recv(n);
    ASE ase(n, true);
    // co_await chl->recv(ase.elem_cnt);
    BlockVec data;
    co_await chl->recvResize(data);
    ase.read(data);
    ase.elem_cnt = 1; //non_empty
    co_return ase;
}

template<typename ASEType>
inline oc::cp::task<> send_ASEs(const std::vector<ASEType>& ases, oc::Socket* chl) {
    size_t n = ases.size();
    co_await chl->send(n);

    std::vector<size_t> ase_sizes(n);
    for (int i = 0; i < n; ++i) ase_sizes[i] = ases[i].n;
    co_await chl->send(ase_sizes);

    BlockVec data;
    for(const auto& ase: ases) ase.write(data);
    co_await chl->send(data);
    co_await chl->flush();
    co_return;
}

inline oc::cp::task<std::vector<ASE> > recv_ASEs(oc::Socket* chl) {
    std::vector<ASE> rs;
    size_t n;
    co_await chl->recv(n);
    std::vector<size_t> ase_sizes(n);
    co_await chl->recv(ase_sizes);

    size_t sum = 0;
    for (int i = 0; i < n; ++i) sum += ase_sizes[i];
    BlockVec data(sum);
    co_await chl->recv(data);

    auto data_span = std::span{data};
    for (int i = 0, idx = 0; i < n; ++i) {
        ASE ase(data_span.subspan(idx, ase_sizes[i]));
        ase.elem_cnt = 1; //non_empty
        idx += ase_sizes[i];
        rs.push_back(std::move(ase));
    }
    co_return rs;
}


template<typename ASEType>
inline oc::cp::task<std::vector<ASE> > send_recv_ASEs(const std::vector<ASEType>& ases, oc::Socket* chl) {
    auto [s1, s2] = co_await oc::cp::when_all_ready(send_ASEs(ases, chl), recv_ASEs(chl));
    s1.result();
    co_return s2.result();
}

inline oc::cp::task<> send_blocks(const BlockVec& blocks, oc::Socket* chl) {
    co_await chl->send(blocks.size());
    // co_await chl->send(ase.elem_cnt);
    if(blocks.size() > 0) co_await chl->send(blocks);
    co_return;
}

inline oc::cp::task<BlockVec> recv_blocks(oc::Socket* chl) {
    size_t n;
    BlockVec rs;
    co_await chl->recv(n);
    if(n) co_await chl->recvResize(rs);
    co_return rs;
}

inline oc::cp::task<> send_OPRF(const OPRFValueVec& values, oc::Socket* chl) {
    BlockVec tmp;
    for (auto x: values) {
        tmp.push_back(x.first);
        tmp.push_back(x.second);
    }
    co_await chl->send(tmp);
    co_return;
}

inline oc::cp::task<OPRFValueVec> recv_OPRF(oc::Socket* chl) {
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