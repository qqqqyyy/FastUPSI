#ifndef PlainASE_H
#define PlainASE_H
#include "ASE.h"

namespace upsi {

class PlainASE : public ASE{
    public:
        int elem_cnt;
        PlainASE(size_t _n) : ASE(_n, true) {elem_cnt = 0;}
        void clear() override {elem_cnt = 0;}
        BlockVec getElements() override;
        bool addElement(const Element &elem) override;
        void pad(oc::PRNG* prng) override;
};

} // namespace upsi

#endif
