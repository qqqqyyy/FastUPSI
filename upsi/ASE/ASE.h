#ifndef ASE_H
#define ASE_H
#include "../utils.h"

namespace upsi {

class ASE{
    public:
    BlockVec ase;
    int n; //size of ase
    int elem_cnt = 0; //number of elements

    ASE(){}
    ASE(const BlockSpan& vec) {
        n = vec.size();
        ase.reserve(n);
        // for (const auto& elem : vec) ase.push_back(std::make_shared<oc::block>(elem));
        ase.insert(ase.end(), vec.begin(), vec.end());
    }
    ASE(int _n, bool build = true) : n(_n) {
        ase.reserve(n);
        // if(build) std::fill(ase.begin(), ase.end(), oc::ZeroBlock);
        // if(build) std::memset(ase.data(), 0, ase.size() * sizeof(oc::block));
        if(build) {
            for (int i = 0; i < n; ++i) {
                // if(build) ase.push_back(std::make_shared<oc::block>(oc::ZeroBlock));
                // else ase.push_back(nullptr);
                ase.push_back(oc::block(0));
            }
        }
    }

    ASE(const BlockVec& vec) {
        n = vec.size();
        elem_cnt = 0;
        ase = vec;
    }

    ASE(BlockVec&& vec) : ase(std::move(vec)) {
        n = ase.size();
        elem_cnt = 0;
    }

    oc::AlignedUnVector<oc::block> VecF() {
        oc::AlignedUnVector<oc::block> out(n);
        for (int i = 0; i < n; ++i) out[i] = (*this)[i];
        return out;
    }

    void write(BlockVec& data) const {
        // data.push_back(oc::toBlock(n));
        // data.push_back(oc::toBlock(elem_cnt));
        for (int i = 0; i < n; ++i) {
            // if(!ase[i]) throw std::runtime_error("serialize error!");
            data.push_back((*this)[i]);
        }
    }

    int read(const BlockSpan& data) {
        int cnt = 0;
        // n = data[cnt++].get<uint64_t>()[0];
        // elem_cnt = data[cnt++].get<uint64_t>()[0];
        for (int i = 0; i < n; ++i) (*this)[i] = data[cnt++];
        return cnt;
    }

    virtual ~ASE() = default;

    virtual void clear() {throw std::runtime_error("clear() not implemented");}

    virtual bool isEmpty() {throw std::runtime_error("isEmpty() not implemented");}

    virtual void copy(const ASE& other_ASE) {
        n = other_ASE.n;
        elem_cnt = other_ASE.elem_cnt;
        if(n != ase.size()) ase.resize(n);
        for (int i = 0; i < n; ++i) (*this)[i] = other_ASE[i];
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
        int cnt = n;
        if(cnt != rhs.n) throw std::runtime_error("ASE::operator +/- size");
        ASE rs(n, true);
        for (int i = 0; i < cnt; ++i) rs[i] = (*this)[i] ^ rhs[i];
        return rs;
    }

    ASE& operator += (const ASE& rhs) {
        int cnt = n;
        if(cnt != rhs.n) throw std::runtime_error("ASE::operator +=/-= size");
        for (int i = 0; i < cnt; ++i) (*this)[i] ^= rhs[i];
        return *this;
    }

    virtual oc::block& operator [] (const size_t& idx) {
        // if(idx >= n) throw std::runtime_error("ASE::operator [] index out of range");
        // if(!ase[idx]) throw std::runtime_error("ASE::operator [] nullptr");
        // return *(ase[idx]);
        return ase[idx];
    }
    virtual const oc::block& operator [] (const size_t& idx) const{
        return ase[idx];
    }

    ASE operator - (const ASE& rhs) {
        return *this + rhs;
    }

    ASE& operator -= (const ASE& rhs) {
        return (*this += rhs);
    }

    ASE operator * (const oc::block& rhs) {
        int cnt = n;
        ASE rs(cnt, true);
        for (int i = 0; i < cnt; ++i) rs[i] = (*this)[i].gf128Mul(rhs);
        return rs;
    }

    ASE& operator *= (const oc::block& rhs) {
        int cnt = n;
        for (int i = 0; i < cnt; ++i) (*this)[i] = (*this)[i].gf128Mul(rhs);
        return *this;
    }

};

} //namespace upsi
#endif