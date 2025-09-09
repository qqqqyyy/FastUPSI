#ifndef rb_okvs_H
#define rb_okvs_H
#include "../ASE/ASE.h"

namespace upsi {

class rb_okvs : public ASE{
    public:
        // _n: ASE size (number of blocks/gf128 elements), I need it to reserve enough space when rb_okvs is constructed
        // put data in ase vector (see ASE.h)
        // true/false: initilize with zeros/null pointers
        rb_okvs(size_t _n) : ASE(_n, true) {elem_cnt = 0;} 
        rb_okvs(ASE&& other_ASE) { //convert basic ASE type to rb_okvs, after which we will use eval
            ase = std::move(other_ASE.ase); //move
            n = ase.size();
        }
        void clear() override {elem_cnt = 0;} //you might change this function if you want
        bool isEmpty() override {return elem_cnt == 0;} //you might change this function if you want
        void build(const std::vector<Element>& elems, oc::block ro_seed = oc::ZeroBlock) override; //encode, build a new okvs 
        void eval(Element elem, BlockVec& values) override; //decode, outputs should be put in values 
        oc::block eval1(Element elem) override;
};

} // namespace upsi

#endif
