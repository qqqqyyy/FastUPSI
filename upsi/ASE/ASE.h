#ifndef ASE_H
#define ASE_H
#include "../utils.h"

namespace upsi {

class ASE{
    public:
    PtrVec ase;
    size_t n; //size of ase
    size_t elem_cnt; //number of elements

    ASE(){}
    ASE(const std::span<oc::block>& vec) {
        n = vec.size();
        ase.reserve(n);
        for (const auto& elem : vec) ase.push_back(std::make_shared<oc::block>(elem));
    }
    ASE(int _n, bool build = true) : n(_n) {
        ase.reserve(n);
        for (int i = 0; i < n; ++i) 
            if(build) ase.push_back(std::make_shared<oc::block>(oc::ZeroBlock));
            else ase.push_back(nullptr);
    }

    virtual ~ASE() = default;

    virtual void clear() {throw std::runtime_error("clear() not implemented");}

    virtual bool isEmpty() {throw std::runtime_error("isEmpty() not implemented");}

    virtual void copy(const ASE& other_ASE) {
        n = other_ASE.n;
        elem_cnt = other_ASE.elem_cnt;
        if(n != ase.size()) ase.resize(n);
        for (int i = 0; i < n; ++i) *(ase[i]) = *(other_ASE.ase[i]);
    }


    // construct this ASE with elements
    virtual void build(const std::vector<Element>& elems, oc::PRNG* prng = nullptr) {throw std::runtime_error("build() not implemented");}

    // Extract elements
    virtual void getElements(std::vector<Element>& elems) {throw std::runtime_error("Extracting elements not supported");}

    // Insert an element to ASE, return true if success, false if it's already full
    virtual bool insertElement(const Element &elem, oc::PRNG* prng = nullptr) {throw std::runtime_error("Adding element not supported");}

    virtual std::vector<std::shared_ptr<ASE> > insert(const std::vector<Element>& elem, oc::PRNG* prng = nullptr) {
        throw std::runtime_error("Adding elements not supported");
    }
    // pad ASE with padding elements
    virtual void pad(oc::PRNG* prng) {throw std::runtime_error("Padding not supported");}

    // output k values (relaxed)
    virtual void eval(Element elem, BlockVec& values) {
        throw std::runtime_error("relaxed ASE eval not supported");
    }

    // output 1 value
    virtual oc::block eval1(Element elem) {
        throw std::runtime_error("ASE eval not supported for single ouput");
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