#ifndef VOLE_H
#define VOLE_H
#include "libOTe/Vole/Silent/SilentVoleSender.h"
#include "libOTe/Vole/Silent/SilentVoleReceiver.h"
#include "libOTe/TwoChooseOne/Silent/SilentOtExtSender.h"
#include "libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h"

#include "utils.h"
#include "network.h"
#include "ASE/ASE.h"

namespace upsi{

class VOLE{
    /*
        Sender: b, delta
        Receiver: a, c
        a = b + delta * c
    */
    public:
        using VecF = typename oc::CoeffCtxGF128::template Vec<oc::block>;
        oc::Socket* chl;
        oc::PRNG* prng;

        size_t n = 1 << 14; 
        size_t m = n - 128; //TODO

        size_t idx = 0;
        VOLE(oc::Socket* _chl = nullptr, oc::PRNG* _prng = nullptr):chl(_chl), prng(_prng){}

        void setup(oc::Socket* _chl, oc::PRNG* _prng = nullptr){
            this->chl = _chl;
            this->prng = _prng;
            idx = 0;
        }

        virtual void generate(size_t vole_size) = 0;
    
};

class VoleSender : public VOLE{
    public:
        VecF b;
        oc::block delta;

        oc::SilentVoleSender<oc::block, oc::block, oc::CoeffCtxGF128> sender;

        oc::RegularPprfSender<oc::block, oc::CoeffCtxGF128> pprf_sender;

        oc::SilentOtExtSender ot_sender;

        using VOLE::VOLE;

        // generate random vole correlations
        void generate(size_t vole_size) override;

        ASE get(int vole_size) { //get b
            ASE cur_b = ASE(b.subspan(idx, vole_size));
            idx += vole_size;
            return cur_b;
        }

        // generate vole with specific a vector from PPRF
        ASE generate(int domain_size, int point_cnt);

};

class VoleReceiver: public VOLE{
    public:
        VecF a, c;

        oc::SilentVoleReceiver<oc::block, oc::block, oc::CoeffCtxGF128> receiver;

        oc::RegularPprfReceiver<oc::block, oc::CoeffCtxGF128> pprf_receiver;

        oc::SilentOtExtReceiver ot_receiver;

        using VOLE::VOLE;

        // generate random vole correlations
        void generate(size_t vole_size) override;

        std::pair<ASE, ASE> get(int vole_size) { //get a, c
            ASE cur_a = ASE(a.subspan(idx, vole_size));
            ASE cur_c = ASE(c.subspan(idx, vole_size));
            idx += vole_size;
            return std::make_pair(cur_a, cur_c);
        }


        // generate vole with specific a vector from PPRF
        ASE generate(int domain_size, BlockVec values, std::vector<size_t> points);
};

oc::BitVector Points2Bits(const std::vector<size_t>& points, int depth);

}

#endif