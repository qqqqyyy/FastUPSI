#ifndef HashTable_H
#define HashTable_H
#include "ASE.h"

namespace upsi {

struct hash_size_table final{
    static size_t bucket_size(size_t x) {
        if(x < 1 << 13) return 6;
        else if(x < 1 << 17) return 5;
        else return 4;
    }
    static size_t bucket_cnt(size_t x) {
        x = std::fmax(x, 1 << 10);
        return 1.2 * x;
    }
    static size_t get(size_t x) {
        return bucket_cnt(x) * bucket_size(x);
    }
};

class HashTable : public ASE{
    oc::block hash_seed;
    public:
        size_t table_capacity;
        size_t bucket_size;
        size_t table_size; //number of buckets
        std::map<Element, size_t> elem_pos_map;
        std::vector<size_t> bucket_elem_cnt;
        explicit HashTable(int n) : ASE(hash_size_table::get(n), true) {
            table_capacity = n;
            bucket_size = hash_size_table::bucket_size(n);
            table_size = hash_size_table::bucket_cnt(n);
            bucket_elem_cnt.resize(table_size, 0);
        }

        explicit HashTable(ASE&& other_ASE){ 
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
        void setup(oc::block ro_seed, size_t _capacity) {
            hash_seed = random_oracle(ro_seed, oc::ZeroBlock);
            table_capacity = _capacity;
            bucket_size = hash_size_table::bucket_size(_capacity);
            table_size = hash_size_table::bucket_cnt(_capacity);
        }

        std::pair<size_t, size_t> getBuckets(const Element& elem) {
            oc::block hash_value = random_oracle(elem, hash_seed);
            auto values = hash_value.get<oc::u64>();
            return std::make_pair(values[0] % table_size, values[1] % table_size);
        }

        void clear() override {elem_cnt = 0;}
        bool isEmpty() override {return elem_cnt == 0;}
        void build(const std::vector<Element>& elems, oc::block ro_seed = oc::ZeroBlock) override; //TODO
        int findPos(const Element& elem, bool remove) override; 
            //remove=true means: 
            // we want to remove the element from the table, but this is a fake deletion
            // we will replace the element with zero (outside this function)
        void eval(Element elem, BlockVec& values) override;
        
};

} // namespace upsi

#endif
