#ifndef ASE_H
#define ASE_H
#include "utils.h"

namespace upsi {

class ASE{
    public:
    PtrVec ase;

    ASE(){}
    ASE(const oc::AlignedUnVector<oc::block>& vec) {
        ase.reserve(vec.size());
        for (const auto& elem : vec) ase.push_back(std::make_shared<oc::block>(elem));
    }
    virtual ~ASE() = default;

    //output k values (relaxed)
    virtual void eval(Element elem, BlockVec& values) {
        std::runtime_error("relaxed ASE eval not supported");
    }

    //output 1 value
    virtual oc::block eval1(Element elem) {
        std::runtime_error("ASE eval not supported for single ouput");
        return oc::ZeroBlock;
    }

    ASE operator + (const ASE& other_ASE) {
        ASE rs;
        int cnt = ase.size();
        rs.ase.reserve(cnt);
        for (int i = 0; i < cnt; ++i) rs.ase.push_back(std::make_shared<oc::block>(*(ase[i]) ^ *(other_ASE.ase[i])));
        return rs;
    }

    ASE operator - (const ASE& other_ASE) {
        return *this + other_ASE;
    }

    ASE operator * (const oc::block& scalar) {
        ASE rs;
        int cnt = ase.size();
        rs.ase.reserve(cnt);
        for (int i = 0; i < cnt; ++i) rs.ase.push_back(std::make_shared<oc::block>((*(ase[i])).gf128Mul(scalar)));
        return rs;
    }
};

} //namespace upsi
#endif