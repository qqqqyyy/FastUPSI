#include "adaptive.h"

namespace upsi{
    
template<typename BaseType>
void Adaptive<BaseType>::addASE() {
    int node_size = start_size * (1 << node_cnt);
    ++node_cnt;
    std::shared_ptr<BaseType> cur_node;
    if constexpr (std::is_same_v<BaseType, rb_okvs>) {
        cur_node = std::make_shared<rb_okvs>(node_size);
    }
    else if constexpr (std::is_same_v<BaseType, HashTable>) {
        cur_node = std::make_shared<HashTable>(node_size);
    }
    else cur_node = std::make_shared<BaseType>(node_size);
    nodes.push_back(cur_node);
    seeds.push_back(oc::sysRandomSeed());
    n += cur_node->n;
    // ase.reserve(ase.size() + cur_node->n);
    // for (int i = 0; i < cur_node->n; ++i) ase.push_back(cur_node->ase[i]);
}

template<typename BaseType>
std::pair<std::vector<std::shared_ptr<BaseType> >, std::vector<int> > Adaptive<BaseType>::insert(const std::vector<Element> &elem, BlockVec& new_seeds) {
    int new_elem_cnt = elem.size();
    while((1 << node_cnt) * start_size <= elem_cnt + new_elem_cnt) addASE();
    int last = elem_cnt / start_size, now = (elem_cnt + new_elem_cnt) / start_size;
    std::vector<Element> all_elems;
    for (int i = node_cnt - 1; i >= 0; --i) 
        if(((last >> i) & 1) == 1 && ((now >> i) & 1) == 0) {
            nodes[i + 1]->getElements(all_elems);
            nodes[i + 1]->clear();
        }
    nodes[0]->getElements(all_elems);
    nodes[0]->clear();
    for (const Element& cur_elem: elem) all_elems.push_back(cur_elem);

    int start = 0;
    for (int i = node_cnt - 1; i >= 0; --i) 
        if(((last >> i) & 1) == 0 && ((now >> i) & 1) == 1) {
            size_t cur_size = start_size * (1 << i);
            size_t end = std::min(start + cur_size, (size_t)all_elems.size());
            if(start < end) {
                std::vector<Element> tmp(all_elems.begin() + start, all_elems.begin() + end);
                nodes[i + 1]->build(tmp, seeds[i + 1] = oc::sysRandomSeed());
            }
            else 
                nodes[i + 1]->build(std::vector<Element>(), seeds[i + 1] = oc::sysRandomSeed());
            start = end;
        }
    if(start < all_elems.size()) {
        std::vector<Element> tmp(all_elems.begin() + start, all_elems.end());
        nodes[0]->build(tmp, seeds[0] = oc::sysRandomSeed());
    }
    else {
        nodes[0]->build(std::vector<Element>(), seeds[0] = oc::sysRandomSeed());
    }
    // nodes[0]->pad();
    elem_cnt += new_elem_cnt;
    
    std::vector<std::shared_ptr<BaseType> > rs;
    std::vector<int> ind;
    rs.push_back(nodes[0]);
    new_seeds.push_back(seeds[0]);
    ind.push_back(0);
    
    for (int i = 0; i < node_cnt; ++i) 
        if(((last >> i) & 1) == 0 && ((now >> i) & 1) == 1) {
            rs.push_back(nodes[i + 1]);
            new_seeds.push_back(seeds[i + 1]);
            ind.push_back(i + 1);
        }
    return std::make_pair(rs, ind);
}


template<typename BaseType>
std::vector<int> Adaptive<BaseType>::update(int new_elem_cnt) {
    while((1 << node_cnt) * start_size <= elem_cnt + new_elem_cnt) addASE();
    int last = elem_cnt / start_size, now = (elem_cnt + new_elem_cnt) / start_size;
    std::vector<int> rs;
    rs.push_back(0);
    for (int i = 0; i < node_cnt; ++i) {
        if(((last >> i) & 1) == 0 && ((now >> i) & 1) == 1)
            rs.push_back(i + 1);
        else if (((last >> i) & 1) == 1 && ((now >> i) & 1) == 0) 
            nodes[i + 1]->clear();
    }
    elem_cnt += new_elem_cnt;
    return rs;
}

template<>
std::pair<std::vector<int>, std::vector<size_t> > Adaptive<PlainASE>::findPos2(
    const std::vector<Element>& elems,
    bool remove
) {
    throw std::runtime_error("findPos for PlainASE adaptive structure not supported");
    return std::pair<std::vector<int>, std::vector<size_t> >();
}

template<>
std::pair<std::vector<int>, std::vector<size_t> > Adaptive<rb_okvs>::findPos2(
    const std::vector<Element>& elems,
    bool remove
) {
    throw std::runtime_error("findPos for rb_okvs adaptive structure not supported");
    return std::pair<std::vector<int>, std::vector<size_t> >();
}



template<>
std::pair<std::vector<int>, std::vector<size_t> > Adaptive<HashTable>::findPos2(
    const std::vector<Element>& elems,
    bool remove
) {
    std::vector<int> node_indices;
    std::vector<size_t> global_positions;
    
    for (const Element& elem : elems) {
        size_t offset = 0;
        bool found = false;
        
        for (int i = 0; i <= node_cnt; ++i) {
            if (!nodes[i]->isEmpty()) {
                int local_pos = nodes[i]->findPos(elem, remove);
                if (local_pos >= 0) {
                    node_indices.push_back(i);
                    global_positions.push_back(offset + local_pos);
                    found = true;
                    break;
                    //we do not change elem_cnt here, because in insert() we assume we never remove elements
                }
            }
            offset += nodes[i]->n;
        }
        if (!found) throw std::runtime_error("Element not found in HashTable adaptive structure");
    }
    
    return std::make_pair(node_indices, global_positions);
}


template<> 
void Adaptive<rb_okvs>::eval_oprf(Element elem, oc::block delta, OPRFValueVec& values) {
    OPRF<rb_okvs> oprf_okvs; 
    for (int i = 0; i <= node_cnt; ++i) 
        if(!nodes[i]->isEmpty()) {
            values.push_back(oprf_okvs.sender(elem, i, *nodes[i], delta, seeds[i]));
            // std::cout << elem << "\t" << i << "\t" << seeds[i] << std::endl;
        }
}

template<> 
void Adaptive<HashTable>::eval_oprf(Element elem, oc::block delta, OPRFValueVec& values) {
    OPRF<HashTable> oprf_hash; 
    for (int i = 0; i <= node_cnt; ++i) 
        if(!nodes[i]->isEmpty()) {
            oprf_hash.sender_relaxed(elem, i, *nodes[i], delta, values, seeds[i]);
            // std::cout << elem << "\t" << i << "\t" << seeds[i] << std::endl;
        }
}

template<>
void Adaptive<PlainASE>::eval_oprf(Element elem, oc::block delta, OPRFValueVec& values) {
	throw std::runtime_error("eval for PlainASE adaptive structure");
}

template class Adaptive<PlainASE>;
template class Adaptive<rb_okvs>;
template class Adaptive<HashTable>;
}