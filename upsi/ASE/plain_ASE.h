#ifndef PlainASE_H
#define PlainASE_H
#include "ASE.h"

namespace upsi {

class PlainASE : public ASE{
    public:
        PlainASE(size_t _n) : ASE(_n, true) {elem_cnt = 0;}
        void clear() override {elem_cnt = 0;}
        bool isEmpty() override {return elem_cnt == 0;}
        void build(const std::vector<Element>& elems, oc::block ro_seed = oc::ZeroBlock, oc::PRNG* prng = nullptr) override;
        void getElements(std::vector<Element>& elem) override;
        bool insertElement(const Element &elem) override;
        void pad(oc::PRNG* prng);
};

} // namespace upsi

#endif
