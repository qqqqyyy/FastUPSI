#include "rb_okvs.h"

namespace upsi {

void rb_okvs::build(const std::vector<Element>& elems, oc::block ro_seed) {
    //TODO
}

// results should be pushed into values (using values.push_back())
// values might not be empty before calling this function
void rb_okvs::eval(Element elem, BlockVec& values) {
    values.push_back(eval1(elem));
}

oc::block rb_okvs::eval1(Element elem) {
    //TODO

    
}

}