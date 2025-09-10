#include "adaptive.h"

namespace upsi{
    
template<typename BaseType>
void Adaptive<BaseType>::addASE() {
    size_t node_size = start_size * (1 << node_cnt);
    ++node_cnt;
    std::shared_ptr<BaseType> cur_node;
    if constexpr (std::is_same_v<BaseType, rb_okvs>) {
        cur_node = std::make_shared<rb_okvs>(rb_okvs_size_table::get(node_size));
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
            int cur_size = start_size * (1 << i);
            std::vector<Element> tmp(all_elems.begin() + start, all_elems.begin() + start + cur_size);
            nodes[i + 1]->build(tmp, seeds[i + 1] = oc::sysRandomSeed());
            start += cur_size;
        }
    std::vector<Element> tmp(all_elems.begin() + start, all_elems.end());
    nodes[0]->build(tmp, seeds[0] = oc::sysRandomSeed());
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
void Adaptive<rb_okvs>::eval_oprf(Element elem, oc::block delta, OPRFValueVec& values) {
    OPRF<rb_okvs> oprf_okvs; 
    for (int i = 0; i <= node_cnt; ++i) 
        if(!nodes[i]->isEmpty()) {
            values.push_back(oprf_okvs.sender(elem, i, *nodes[i], delta, seeds[i]));
            // std::cout << elem << "\t" << i << "\t" << seeds[i] << std::endl;
        }
}

template<>
void Adaptive<PlainASE>::eval_oprf(Element elem, oc::block delta, OPRFValueVec& values) {
	throw std::runtime_error("eval for PlainASE adaptive structure");
}

template class Adaptive<PlainASE>;
template class Adaptive<rb_okvs>;

}