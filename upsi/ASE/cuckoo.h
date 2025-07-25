#ifndef Cuckoo_H
#define Cuckoo_H
#include "ASE.h"

namespace upsi{

class Cuckoo : public ASE{

    public: 
        size_t table_size, stash_size;
        Cuckoo(size_t _table_size = DEFAULT_CUCKOO_SIZE, size_t _stash_size = 0):
            ASE(_table_size + _stash_size, true){
                table_size = _table_size; 
                stash_size = _stash_size;
            }

        Cuckoo(ASE&& other_ASE) { //move
            ase = std::move(other_ASE.ase);
            //TODO: table_size, stash_size
        }

        void clear() override {for (int i = 0; i < table_size + stash_size; ++i) *(ase[i]) = oc::ZeroBlock;}

        void eval(Element elem, BlockVec& values) override;

};

}

#endif