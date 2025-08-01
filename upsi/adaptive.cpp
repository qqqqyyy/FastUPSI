#include "adaptive.h"

namespace upsi{
    
template<typename BaseType>
void Adaptive<BaseType>::addASE() {
    size_t node_size = start_size * (1 << node_cnt);
    ++node_cnt;
    auto cur_node = std::make_shared<BaseType>(node_size);
    nodes.push_back(cur_node);
    for (int i = 0; i < node_size; ++i) ase.push_back(cur_node->ase[i]);
}

template<typename BaseType>
std::vector<std::shared_ptr<ASE> > Adaptive<BaseType>::insert(const std::vector<Element> &elem, oc::PRNG* prng) {
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
            int cur_size = (1 << i);
            std::vector<Element> tmp(all_elems.begin() + start, all_elems.begin() + start + cur_size);
            nodes[i + 1]->build(tmp, prng);
            start += cur_size;
        }
    std::vector<Element> tmp(all_elems.begin() + start, all_elems.end());
    nodes[0]->build(tmp, prng);
    // nodes[0]->pad();
    elem_cnt += new_elem_cnt;

    std::vector<std::shared_ptr<ASE> > rs;
    rs.push_back(nodes[0]);
    for (int i = 0; i < node_cnt; ++i) 
        if(((last >> i) & 1) == 0 && ((now >> i) & 1) == 1) 
            rs.push_back(nodes[i + 1]);
    return rs;
}

template<typename BaseType>
void Adaptive<BaseType>::replaceASEs(int new_elem_cnt, const std::vector<std::shared_ptr<ASE> >& new_ASEs) {
    while((1 << node_cnt) * start_size <= elem_cnt + new_elem_cnt) addASE();
    int last = elem_cnt / start_size, now = (elem_cnt + new_elem_cnt) / start_size;
    nodes[0]->copy(*new_ASEs[0]);
    
    for (int i = 0, idx = 0; i < node_cnt; ++i) 
        if(((last >> i) & 1) == 0 && ((now >> i) & 1) == 1)
            nodes[i + 1]->copy(*new_ASEs[++idx]);
        else if (((last >> i) & 1) == 1 && ((now >> i) & 1) == 0) 
            nodes[i + 1]->clear();

    elem_cnt += new_elem_cnt;
}

template<typename BaseType> 
void Adaptive<BaseType>::eval(Element elem, BlockVec& values) {
    for (int i = 0; i <= node_cnt; ++i) 
        if(!nodes[i]->isEmpty()) 
            nodes[i]->eval(elem, values);
}

template class Adaptive<PlainASE>;

}