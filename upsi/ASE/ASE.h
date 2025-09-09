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
    ASE(const BlockSpan& vec) {
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

    oc::AlignedUnVector<oc::block> VecF() {
        oc::AlignedUnVector<oc::block> out(n);
        for (size_t i = 0; i < n; ++i) out[i] = *(ase[i]);
        return out;
    }

    void write(BlockVec& data) const {
        // data.push_back(oc::toBlock(n));
        // data.push_back(oc::toBlock(elem_cnt));
        for (int i = 0; i < n; ++i) {
            if(!ase[i]) throw std::runtime_error("serialize error!");
            data.push_back(*ase[i]);
        }
    }

    int read(const BlockSpan& data) {
        int cnt = 0;
        // n = data[cnt++].get<uint64_t>()[0];
        // elem_cnt = data[cnt++].get<uint64_t>()[0];
        for (int i = 0; i < n; ++i) *ase[i] = data[cnt++];
        return cnt;
    }

    virtual ~ASE() = default;

    virtual void clear() {throw std::runtime_error("clear() not implemented");}

    virtual bool isEmpty() {throw std::runtime_error("isEmpty() not implemented");}

    virtual void copy(const ASE& other_ASE) {
        n = other_ASE.n;
        // elem_cnt = other_ASE.elem_cnt;
        if(n != ase.size()) ase.resize(n);
        for (int i = 0; i < n; ++i) *(ase[i]) = *(other_ASE.ase[i]);
    }


    // construct this ASE with elements
    virtual void build(const std::vector<Element>& elems, oc::block ro_seed = oc::ZeroBlock) {throw std::runtime_error("build() not implemented");}

    // Extract elements, only for plain_ASE
    virtual void getElements(std::vector<Element>& elems) {throw std::runtime_error("Extracting elements not supported");}

    // Insert an element to ASE, return true if success, false if it's already full, only for plain_ASE
    virtual bool insertElement(const Element &elem) {throw std::runtime_error("Adding element not supported");}

    // virtual std::vector<std::shared_ptr<ASE> > insert(const std::vector<Element>& elem, oc::PRNG* prng = nullptr) {
    //     throw std::runtime_error("Adding elements not supported");
    // }
    // // pad ASE with padding elements
    // virtual void pad(oc::PRNG* prng) {throw std::runtime_error("Padding not supported");}

    // output k values (relaxed)
    virtual void eval(Element elem, BlockVec& values) {
        throw std::runtime_error("relaxed ASE eval not supported");
    }

    // output 1 value
    virtual oc::block eval1(Element elem) {
        throw std::runtime_error("ASE eval not supported for single ouput");
        return oc::ZeroBlock;
    }

    ASE operator + (const ASE& rhs) {
        int cnt = ase.size();
        if(cnt != rhs.ase.size()) throw std::runtime_error("ASE::operator +/- size");
        ASE rs(n, true);
        for (int i = 0; i < cnt; ++i) *rs.ase[i] = *(ase[i]) ^ *(rhs.ase[i]);
        return rs;
    }

    ASE& operator += (const ASE& rhs) {
        int cnt = ase.size();
        if(cnt != rhs.ase.size()) throw std::runtime_error("ASE::operator +=/-= size");
        for (int i = 0; i < cnt; ++i) *(ase[i]) ^= *(rhs.ase[i]);
        return *this;
    }

    oc::block& operator [] (const size_t& idx) {
        if(idx >= n) throw std::runtime_error("ASE::operator [] index out of range");
        if(!ase[idx]) throw std::runtime_error("ASE::operator [] nullptr");
        return *(ase[idx]);
    }

    ASE operator - (const ASE& rhs) {
        return *this + rhs;
    }

    ASE& operator -= (const ASE& rhs) {
        return (*this += rhs);
    }

    ASE operator * (const oc::block& rhs) {
        ASE rs;
        int cnt = ase.size();
        rs.ase.reserve(cnt);
        for (int i = 0; i < cnt; ++i) rs.ase.push_back(std::make_shared<oc::block>((ase[i])->gf128Mul(rhs)));
        return rs;
    }

    ASE& operator *= (const oc::block& rhs) {
        int cnt = ase.size();
        for (int i = 0; i < cnt; ++i) *(ase[i]) = (ase[i])->gf128Mul(rhs);
        return *this;
    }

};

} //namespace upsi
#endif