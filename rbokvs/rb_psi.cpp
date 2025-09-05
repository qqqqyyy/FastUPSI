// rb_psi.cpp — Real TCP PSI (Fig.9) with trivial sub-VOLE
// - columns match via kv_cnt from receiver
// - sender uses batch decode for Y
// SECURITY: sub-VOLE here is a placeholder.

#include "rb_psi.hpp"

#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <iostream>

#include <boost/asio.hpp>


#include "types.hpp"  // FE ops

// set to 1 to enable extra runtime checks / prints
#ifndef RBPSI_CHECKS
#define RBPSI_CHECKS 1
#endif

namespace rbpsi {

using FE = rbokvs::FE2_128;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// // ---------- hash helpers ----------
// static inline std::array<uint8_t,32> sha3_256_bytes(const uint8_t* d, size_t n) {
//     std::array<uint8_t,32> h{};
//     CryptoPP::SHA3_256 sh; sh.Update(d, n); sh.Final(h.data()); return h;
// }
// ---- tiny SHA3-256 (Keccak-f[1600], 24 rounds) ----
namespace tinysha3 {
    static inline uint64_t rol(uint64_t x, unsigned n){ return (x<<n) | (x>>(64-n)); }
    static void keccakf(uint64_t s[25]){
        static const uint64_t RC[24]={
            0x0000000000000001ULL,0x0000000000008082ULL,0x800000000000808aULL,0x8000000080008000ULL,
            0x000000000000808bULL,0x0000000080000001ULL,0x8000000080008081ULL,0x8000000000008009ULL,
            0x000000000000008aULL,0x0000000000000088ULL,0x0000000080008009ULL,0x000000008000000aULL,
            0x000000008000808bULL,0x800000000000008bULL,0x8000000000008089ULL,0x8000000000008003ULL,
            0x8000000000008002ULL,0x8000000000000080ULL,0x000000000000800aULL,0x800000008000000aULL,
            0x8000000080008081ULL,0x8000000000008080ULL,0x0000000080000001ULL,0x8000000080008008ULL};
        static const unsigned r[24]={1,3,6,10,15,21,28,36,45,55,2,14,27,41,56,8,25,43,62,18,39,61,20,44};
        for(int round=0;round<24;++round){
            uint64_t C[5],D[5];
            for(int x=0;x<5;++x) C[x]=s[x]^s[x+5]^s[x+10]^s[x+15]^s[x+20];
            for(int x=0;x<5;++x) D[x]=C[(x+4)%5]^rol(C[(x+1)%5],1);
            for(int x=0;x<5;++x) for(int y=0;y<5;++y) s[x+5*y]^=D[x];
            uint64_t cur=s[1]; int x=1,y=0;
            for(int i=0;i<24;++i){ int X=y,Y=(2*x+3*y)%5,idx=X+5*Y; uint64_t tmp=s[idx]; s[idx]=rol(cur,r[i]); cur=tmp; x=X; y=Y; }
            for(int y0=0;y0<5;++y0){ uint64_t row[5]; for(int x0=0;x0<5;++x0) row[x0]=s[x0+5*y0];
                for(int x0=0;x0<5;++x0) s[x0+5*y0]=row[x0]^((~row[(x0+1)%5])&row[(x0+2)%5]); }
            s[0]^=RC[round];
        }
    }
    static inline void sha3_256(const uint8_t* in,size_t inlen,uint8_t out[32]){
        uint64_t st[25]{}; const size_t rate=136;
        while(inlen>=rate){ for(size_t i=0;i<rate/8;++i){ uint64_t w=0; std::memcpy(&w,in+8*i,8); st[i]^=w; } keccakf(st); in+=rate; inlen-=rate; }
        uint8_t last[rate]; std::memset(last,0,rate);
        if(inlen) std::memcpy(last,in,inlen); last[inlen]^=0x06; last[rate-1]^=0x80;
        for(size_t i=0;i<rate/8;++i){ uint64_t w=0; std::memcpy(&w,last+8*i,8); st[i]^=w; }
        keccakf(st); for(size_t i=0;i<4;++i) std::memcpy(out+8*i,&st[i],8);
    }
    } // tinysha3
    
    static inline std::array<uint8_t,32> sha3_256_bytes(const uint8_t* d, size_t n){
        std::array<uint8_t,32> h{}; tinysha3::sha3_256(d,n,h.data()); return h;
    }
    

static inline FE HB(const FE& x, const std::array<uint8_t,16>& rp) {
    auto xb = x.to_bytes_le();               // 32B
    std::array<uint8_t,32> buf{};
    std::memcpy(buf.data(), xb.data(), xb.size());
    std::memcpy(buf.data()+xb.size(), rp.data(), rp.size());
    auto h = sha3_256_bytes(buf.data(), buf.size());
    return FE::from_bytes_le(h.data(), h.size());
}
static inline std::array<uint8_t,32> Ho(const FE& v) {
    auto vb = v.to_bytes_le();
    return sha3_256_bytes(vb.data(), vb.size());
}

// ---------- rng ----------
static inline std::array<uint8_t,16> rnd16(std::mt19937_64& g) {
    std::array<uint8_t,16> a{}; for (auto& b : a) b = static_cast<uint8_t>(g()); return a;
}
static inline FE rnd_fe(std::mt19937_64& g) {
    uint8_t b[16]; for (auto& x : b) x = static_cast<uint8_t>(g()); return FE::from_bytes_le(b, 16);
}

// ---------- wire I/O ----------
static inline void send_all(tcp::socket& s, const void* p, size_t n) { asio::write(s, asio::buffer(p, n)); }
static inline void recv_all(tcp::socket& s, void* p, size_t n)       { asio::read(s,  asio::buffer(p, n)); }
static inline void send_u32(tcp::socket& s, uint32_t v){ send_all(s,&v,4);}  static inline uint32_t recv_u32(tcp::socket& s){uint32_t v;recv_all(s,&v,4);return v;}
static inline void send_arr16(tcp::socket& s, const std::array<uint8_t,16>& a){ send_all(s,a.data(),16);} static inline void recv_arr16(tcp::socket& s, std::array<uint8_t,16>& a){ recv_all(s,a.data(),16); }
static inline void send_arr32(tcp::socket& s, const std::array<uint8_t,32>& a){ send_all(s,a.data(),32);} static inline void recv_arr32(tcp::socket& s, std::array<uint8_t,32>& a){ recv_all(s,a.data(),32); }

static inline void send_fe(tcp::socket& s, const FE& x){ auto b=x.to_bytes_le(); send_all(s,b.data(),b.size()); }
static inline FE  recv_fe(tcp::socket& s){ std::array<uint8_t,16> b{}; recv_all(s,b.data(),b.size()); return FE::from_bytes_le(b.data(),b.size()); }
static inline void send_fev(tcp::socket& s, const std::vector<FE>& v){ send_u32(s,(uint32_t)v.size()); for (auto& e:v) send_fe(s,e); }
static inline std::vector<FE> recv_fev(tcp::socket& s){ uint32_t n=recv_u32(s); std::vector<FE> v(n); for(uint32_t i=0;i<n;++i)v[i]=recv_fe(s); return v; }
static inline void send_tagv(tcp::socket& s, const std::vector<std::array<uint8_t,32>>& v){ send_u32(s,(uint32_t)v.size()); for(auto&t:v) send_arr32(s,t); }
static inline std::vector<std::array<uint8_t,32>> recv_tagv(tcp::socket& s){ uint32_t n=recv_u32(s); std::vector<std::array<uint8_t,32>> v(n); for(uint32_t i=0;i<n;++i) recv_arr32(s,v[i]); return v; }

// ----------c ----------
struct SVSend { FE d; std::vector<FE> B; };   // sender view
struct SVRecv { std::vector<FE> A, C; };      // receiver view
static inline std::pair<SVSend,SVRecv> subvole_local(size_t m, std::mt19937_64& g){
    FE d=rnd_fe(g); std::vector<FE> A(m),B(m),C(m);
    for(size_t i=0;i<m;++i){ A[i]=rnd_fe(g); B[i]=rnd_fe(g); C[i]=A[i]*d+B[i]; }
    return {SVSend{d,std::move(B)}, SVRecv{std::move(A),std::move(C)}};
}

// ================= Receiver (server) =================
std::vector<FE> psi_recv(uint16_t port, const std::vector<FE>& X){
    std::mt19937_64 g{std::random_device{}()};
    asio::io_context io; tcp::acceptor acc(io,tcp::endpoint(tcp::v4(),port)); tcp::socket s(io); acc.accept(s);

    // Step 1: sample r, build L, encode -> P
    auto r1=rnd16(g), r2=rnd16(g), rp=rnd16(g);
    const uint32_t kv_cnt=(uint32_t)X.size();
    rbokvs::RbOkvsGF2 okvs_enc(kv_cnt, r1, r2);

    std::vector<std::pair<FE,FE>> L; L.reserve(X.size());
    for (auto& x: X) L.emplace_back(x, HB(x,rp));

    std::vector<FE> P = okvs_enc.encode(L);   // m == columns
    const uint32_t m=(uint32_t)P.size();

#if RBPSI_CHECKS
    // quick self-check: decode(P,x) == HB(x,rp)
    auto chk = okvs_enc.decode(P, X);
    for (size_t i=0;i<X.size();++i) {
        if (chk[i] != HB(X[i], rp)) {
            std::cerr << "[rbpsi] encode check failed at i="<<i<<"\n";
            throw std::runtime_error("encode/decode mismatch on receiver");
        }
    }
#endif

    // Step 2: sub-VOLE
    auto [S,R]=subvole_local(m,g);

    // Step 3: send m, kv_cnt, r1,r2,rp, d, B, A'
    std::vector<FE> Ap(m); for(uint32_t i=0;i<m;++i) Ap[i]=R.A[i]-P[i];
    send_u32(s,m); send_u32(s,kv_cnt); send_arr16(s,r1); send_arr16(s,r2); send_arr16(s,rp);
    send_fe(s,S.d); send_fev(s,S.B); send_fev(s,Ap);

    // Step 4: recv tags
    auto Ytags = recv_tagv(s);

    // Step 5: output
    struct H32 { size_t operator()(const std::array<uint8_t,32>& a) const noexcept { size_t h=0; for(uint8_t b:a) h=h*131^b; return h; }};
    std::unordered_set<std::array<uint8_t,32>, H32> T; T.reserve(Ytags.size()*2);
    for(auto& t: Ytags) T.insert(t);

    std::vector<FE> Z = okvs_enc.decode(R.C, X); // batch decode
    std::vector<FE> out; out.reserve(X.size());
    for(size_t i=0;i<X.size();++i) if (T.find(Ho(Z[i]))!=T.end()) out.push_back(X[i]);
    return out;
}

// ================= Sender (client) =================
void psi_send(const std::string& host, uint16_t port, const std::vector<FE>& Y){
    asio::io_context io; tcp::socket s(io);
    s.connect(tcp::endpoint(asio::ip::make_address(host), port));

    const uint32_t m = recv_u32(s);
    const uint32_t kv_cnt = recv_u32(s);  // exact kv count used by encoder
    std::array<uint8_t,16> r1{},r2{},rp{}; recv_arr16(s,r1); recv_arr16(s,r2); recv_arr16(s,rp);
    FE d = recv_fe(s);
    std::vector<FE> B  = recv_fev(s);
    std::vector<FE> Ap = recv_fev(s);

    if (B.size()!=m || Ap.size()!=m) throw std::runtime_error("bad lengths from receiver");

    rbokvs::RbOkvsGF2 okvs_dec(kv_cnt, r1, r2);     // columns now match encoder exactly

    std::vector<FE> Bp(m); for(uint32_t i=0;i<m;++i) Bp[i]=B[i]+Ap[i]*d;

    // ---- batch decode for Y ----
    std::vector<FE> Ty = okvs_dec.decode(Bp, Y);
    if (Ty.size() != Y.size()) throw std::runtime_error("decode(Y) size mismatch");

    std::vector<std::array<uint8_t,32>> tags; tags.reserve(Y.size());
    for (size_t i=0;i<Y.size();++i) {
        // FE cor = HB(Y[i], rp) * d;
        // tags.push_back(Ho(Ty[i] - cor));
        tags.push_back(Ho(Ty[i] + HB(Y[i], rp) * d));

    }
    std::mt19937_64 g{std::random_device{}()}; std::shuffle(tags.begin(), tags.end(), g);

    send_tagv(s, tags);
}

} 
