#ifndef Cuckoo_H
#define Cuckoo_H
#include "crypto_node.h"

namespace upsi{

class Cuckoo : public CryptoNode{

    public: 
        size_t table_size, stash_size;
        Cuckoo(size_t _node_size, size_t _table_size = DEFAULT_CUCKOO_SIZE, size_t _stash_size = 0):
            table_size(_table_size), stash_size(_stash_size){
                node_size = _node_size;
                ase.reserve(_table_size + _stash_size);
                for (int i = 0; i < _table_size + _stash_size; ++i) 
                    ase.push_back(std::make_shared<oc::block>(oc::ZeroBlock));
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