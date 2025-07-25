#include "plain_ASE.h"

namespace upsi {

BlockVec PlainASE::getElements() {
    BlockVec rs;
    for (int i = 0; i < elem_cnt; ++i) rs.push_back(*ase[i]);
    return rs;
}

// Add an element to the node, return true if success, false if it's already full
bool PlainASE::addElement(const Element &elem) {
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
