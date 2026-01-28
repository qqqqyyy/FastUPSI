#include "HashTable.h"

namespace upsi {

void HashTable::build(const std::vector<Element>& elems, oc::block ro_seed) {
    elem_cnt = elems.size();
    hash_seed = random_oracle(ro_seed, oc::ZeroBlock);
    for (int i = 0; i < n; ++i) ase[i] = oc::ZeroBlock;
    for (int i = 0; i < table_size; ++i) bucket_elem_cnt[i] = 0;
    elem_pos_map.clear();
    for (auto& elem: elems) {
        auto buckets = getBuckets(elem);
        size_t pos;
        if(bucket_elem_cnt[buckets.first] < bucket_size && bucket_elem_cnt[buckets.first] <= bucket_elem_cnt[buckets.second]) {
            pos = buckets.first * bucket_size + bucket_elem_cnt[buckets.first];
            ++bucket_elem_cnt[buckets.first];
        }
        else if(bucket_elem_cnt[buckets.second] < bucket_size) {
            pos = buckets.second * bucket_size + bucket_elem_cnt[buckets.second];
            ++bucket_elem_cnt[buckets.second];
        }
        else {
            std::cerr << "capacity: " << table_capacity << ", size: " << table_size << ", bucket_size: " << bucket_size << "\n";
            throw std::runtime_error("HashTable build error: both buckets full");
        }
        if(pos >= n) throw std::runtime_error("HashTable build error: position out of range");
        ase[pos] = random_oracle(elem, ro_seed);
        elem_pos_map[elem] = pos;
    }
}

int HashTable::findPos(const Element& elem, bool remove) {
    if(elem_pos_map.find(elem) != elem_pos_map.end()) {
        int rs = elem_pos_map[elem];
        if(remove) {
            elem_pos_map.erase(elem);
            // ase[rs] = oc::ZeroBlock;
            //we do not change bucket_elem_cnt or elem_cnt here, because in insert() we assume we never remove elements
        }
        return rs;
    }
    return -1; // Not found
}

void HashTable::eval(Element elem, BlockVec& values) {
    auto buckets = getBuckets(elem);
    for (size_t i = 0; i < bucket_size; ++i) {
        size_t pos1 = buckets.first * bucket_size + i;
        size_t pos2 = buckets.second * bucket_size + i;
        values.push_back(ase[pos1]);
        values.push_back(ase[pos2]);
    }
}

} // namespace upsi
