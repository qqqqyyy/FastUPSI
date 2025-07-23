#ifndef ASE_H
#define ASE_H
#include "utils.h"

namespace upsi {

class ASE{
    public:
    BlockVec ase;

    ASE(){}
    virtual ~ASE() = default;

    //output k values (relaxed)
    virtual BlockVec eval(Element elem) {
        std::runtime_error("relaxed ASE eval not supported");
        return BlockVec();
    }

    //output 1 value
    virtual oc::block eval1(Element elem) {
        std::runtime_error("ASE eval not supported for single ouput");
        return oc::ZeroBlock;
    }
};

} //namespace upsi
#endif