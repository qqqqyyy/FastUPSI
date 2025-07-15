#ifndef ASE_H
#define ASE_H
#include "utils.h"

namespace upsi {

class ASE{
    public:
    std::vector<oc::block> ase;
    ASE(){} 

    //output k values (relaxed)
    virtual std::vector<oc::block> eval(Element elem) {
        std::runtime_error("relaxed ASE eval not supported");
        return std::vector<oc::block>();
    }

    //output 1 value
    virtual oc::block eval1(Element elem) {
        std::runtime_error("ASE eval not supported for single ouput");
        return oc::ZeroBlock;
    }
};

} //namespace upsi
#endif