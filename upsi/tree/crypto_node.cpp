#include "crypto_node.h"

namespace upsi {

// Add an element to the node, return true if success, false if it's already full
bool RawNode::addElement(Element &elem) {
    size_t node_vec_size = this->elems.size();
    if (node_vec_size >= this->node_size) {
        return false;
    }
    else {
        this->elems.push_back(elem);
        return true;
    }
}

void RawNode::pad(oc::PRNG* prng) {
    while (elems.size() < node_size) {
        elems.push_back(GetRandomPadElement(prng));
    }
}

} // namespace upsi
