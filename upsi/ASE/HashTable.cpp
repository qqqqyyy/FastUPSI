#include "HashTable.h"

namespace upsi {

void HashTable::insert(Element elem, oc::block ro_seed, int last_pos, int max_iterations) {
    if(max_iterations <= 0) {
        throw std::runtime_error("HashTable insert failed: max iterations reached.");
    }
    
    auto buckets = getBuckets(elem);
    int idx = -1;
    for (int i = 0; i < buckets.size(); ++i) {
        if(last_pos != buckets[i] && bucket_elem_cnt[buckets[i]] < bucket_size) {
            if(idx == -1 || (bucket_elem_cnt[buckets[i]] < bucket_elem_cnt[buckets[idx]]))
                idx = i;
        }
    }
    if(idx != -1) {
        size_t pos = buckets[idx] * bucket_size + bucket_elem_cnt[buckets[idx]];
        ase[pos] = elem;
        elem_pos_map[elem] = pos;
        ++bucket_elem_cnt[buckets[idx]];
        return;
    }

    int kickout_idx = rand() % buckets.size();
    do {
        kickout_idx = rand() % buckets.size();
    } while(last_pos == buckets[kickout_idx]);
    size_t kickout_pos = buckets[kickout_idx] * bucket_size + rand() % bucket_size;
    Element kickout_elem = ase[kickout_pos];
    ase[kickout_pos] = elem;
    elem_pos_map[elem] = kickout_pos;

    insert(kickout_elem, ro_seed, buckets[kickout_idx], max_iterations - 1);
}

void HashTable::build(const std::vector<Element>& elems, oc::block ro_seed) {
    elem_cnt = std::max(1, (int)elems.size()); //pretend the table is not empty
    hash_seed = random_oracle(ro_seed, oc::ZeroBlock);
    for (int i = 0; i < n; ++i) ase[i] = oc::ZeroBlock;
    for (int i = 0; i < table_size; ++i) bucket_elem_cnt[i] = 0;
    elem_pos_map.clear();
    for (auto& elem: elems) 
        insert(elem, ro_seed, -1);
    for (auto& elem: elems) {
        int pos = elem_pos_map[elem];
        if(ase[pos] != elem)
            throw std::runtime_error("HashTable build failed: element not found in the expected position");
        ase[pos] = random_oracle(elem, ro_seed);
    }
}

int HashTable::findPos(const Element& elem, bool remove) {
    if(elem_pos_map.find(elem) != elem_pos_map.end()) {
        int rs = elem_pos_map[elem];
        if(remove) {
            elem_pos_map.erase(elem);
            // do it later: ase[rs] = oc::ZeroBlock;
            //we do not change bucket_elem_cnt or elem_cnt here, because in insert() we assume we never remove elements
        }
        return rs;
    }
    return -1; // Not found
}

void HashTable::eval(Element elem, BlockVec& values) {
    auto buckets = getBuckets(elem);
    for (auto cur_bucket: buckets) {
        for (size_t i = 0; i < bucket_size; ++i) {
            size_t pos = cur_bucket * bucket_size + i;
            values.push_back(ase[pos]);
        }
    }
}

} // namespace upsi
