#ifndef rb_okvs_H
#define rb_okvs_H

#include "../ASE/ASE.h"

namespace upsi {

class rb_okvs : public ASE {
public:
    // n = number of GF(128) table slots
    explicit rb_okvs(size_t n, size_t band_width = 0)
        : ASE(static_cast<int>(n), /*build*/true),
          band_width_(band_width ? band_width : default_band_width(n)) {}

    // “convert” ctor from an already-allocated ASE
    explicit rb_okvs(ASE&& other_ASE)
    {
        ase = std::move(other_ASE.ase);
        n   = ase.size();
        band_width_ = default_band_width(n);
        elem_cnt = 0;
    }

    void clear() override { elem_cnt = 0; }
    bool isEmpty() override { return elem_cnt == 0; }

    // Build / Encode (fills ase[]), rhs derived from random_oracle(elem, ro_seed)
    void build(const std::vector<Element>& elems, oc::block ro_seed = oc::ZeroBlock) override;

    // Decode (relaxed): push one value per query element into `values`
    void eval(Element elem, BlockVec& values) override { values.push_back(eval1(elem)); }

    // Decode single output
    oc::block eval1(Element elem) override;

private:
    // per-instance hashing seeds (must be the same for build/eval)
    oc::block r1_{ oc::ZeroBlock };
    oc::block r2_{ oc::ZeroBlock };

    // band width (local width per row, <= 256)
    size_t band_width_{ 0 };

    static size_t default_band_width(size_t n)
    {
        // heuristic: between 16 and 256
        size_t bw = n / 64;
        if (bw < 16) bw = 16;
        if (bw > 256) bw = 256;
        return bw;
    }
};

} // namespace upsi

#endif