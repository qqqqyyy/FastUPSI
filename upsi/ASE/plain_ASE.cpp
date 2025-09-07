#include "plain_ASE.h"

namespace upsi {

void PlainASE::build(const std::vector<Element>& elems, oc::block ro_seed, oc::PRNG* prng) {
    elem_cnt = elems.size();
    for (int i = 0; i < elem_cnt; ++i) *(ase[i]) = elems[i];
}

void PlainASE::getElements(std::vector<Element>& elems) {
    for (int i = 0; i < elem_cnt; ++i) elems.push_back(*ase[i]);
}

// Add an element to the node, return true if success, false if it's already full
bool PlainASE::insertElement(const Element &elem) {
    if (elem_cnt >= n) {
        return false;
    }
    else {
        *(ase[elem_cnt++]) = elem;
        return true;
    }
}

void PlainASE::pad(oc::PRNG* prng) {
    while (elem_cnt < n) {
        *(ase[elem_cnt++]) = GetRandomPadElement(prng);
    }
}

} // namespace upsi
