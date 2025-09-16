#include "plain_ASE.h"

namespace upsi {

void PlainASE::build(const std::vector<Element>& elems, oc::block ro_seed) {
    elem_cnt = elems.size();
    // for (int i = 0; i < elem_cnt; ++i) *(ase[i]) = elems[i];
    for (int i = 0; i < elem_cnt; ++i) ase[i] = elems[i];
}

void PlainASE::getElements(std::vector<Element>& elems) {
    // for (int i = 0; i < elem_cnt; ++i) elems.push_back(*ase[i]);
    if(elem_cnt) elems.insert(elems.end(), ase.begin(), ase.begin() + elem_cnt);
}

// Add an element to the node, return true if success, false if it's already full
bool PlainASE::insertElement(const Element &elem) {
    if (elem_cnt >= n) {
        return false;
    }
    else {
        // *(ase[elem_cnt++]) = elem;
        ase[elem_cnt++] = elem;
        return true;
    }
}

bool PlainASE::find(const Element &elem, bool remove) {
    bool rs = false; 
    int idx = elem_cnt - 1;
    for (int i = 0; i < elem_cnt; ++i) if(ase[i] == elem) { 
        rs = true; 
        idx = i;
        break;
    }
    if(rs && remove) {
        if(idx != elem_cnt - 1) 
            std::swap(ase[idx], ase[elem_cnt - 1]);
        ase[--elem_cnt] = oc::ZeroBlock;
    }
    return rs;
}

} // namespace upsi
