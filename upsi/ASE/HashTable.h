#ifndef HashTable_H
#define HashTable_H
#include "ASE.h"

namespace upsi {

class HashTable : public ASE{
    public:
        HashTable(int _n) : ASE(_n, true) {elem_cnt = 0;}

        explicit HashTable(ASE&& other_ASE){ //TODO: other parameters?
            ase = std::move(other_ASE.ase);
            n   = ase.size();
            elem_cnt = other_ASE.elem_cnt;
        }
        void copy(const ASE& other_ASE) override{
            n = other_ASE.n;
            elem_cnt = other_ASE.elem_cnt;
            if(n != ase.size()) ase.resize(n);
            // for (int i = 0; i < n; ++i) *(ase[i]) = (*other_ASE)[i];
            for (int i = 0; i < n; ++i) ase[i] = other_ASE[i];
        }

        // setup before eval
        void setup(oc::block ro_seed) {//TODO
        }

        void clear() override {elem_cnt = 0;} //TODO: check
        bool isEmpty() override {return elem_cnt == 0;}
        void build(const std::vector<Element>& elems, oc::block ro_seed = oc::ZeroBlock) override; //TODO
        int findPos(const Element& elem) override;
        //TODO: setup hashing seed?
};

} // namespace upsi

#endif
