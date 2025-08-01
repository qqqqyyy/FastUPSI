#ifndef PlainASE_H
#define PlainASE_H
#include "ASE.h"

namespace upsi {

class PlainASE : public ASE{
    public:
        PlainASE(size_t _n) : ASE(_n, true) {elem_cnt = 0;}
        void clear() override {elem_cnt = 0;}
        virtual void build(const std::vector<Element>& elems, oc::PRNG* prng = nullptr) override;
        void getElements(std::vector<Element>& elem) override;
        bool insertElement(const Element &elem, oc::PRNG* prng = nullptr) override;
        void pad(oc::PRNG* prng) override;
};

} // namespace upsi

#endif
