#include "crypto_node.h"

namespace upsi {

BlockVec RawNode::getElements() {
    BlockVec rs;
    for (int i = 0; i < elem_cnt; ++i) rs.push_back(*ase[i]);
    return rs;
}

// Add an element to the node, return true if success, false if it's already full
bool RawNode::addElement(const Element &elem) {
    if (elem_cnt >= this->node_size) {
        return false;
    }
    else {
        *(ase[elem_cnt++]) = elem;
        return true;
    }
}

void RawNode::pad(oc::PRNG* prng) {
    while (elem_cnt < node_size) {
        *(ase[elem_cnt++]) = GetRandomPadElement(prng);
    }
}

} // namespace upsi
