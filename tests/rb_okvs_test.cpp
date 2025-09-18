#include <cassert>
#include <iostream>

#include "../upsi/utils.h"
#include "../upsi/ASE/ASE.h"
#include "../upsi/rbokvs/rb_okvs.h"

using namespace upsi;

static void one_run(size_t set_size)
{
    std::cout << "\n=== rb_okvs test: set=" << set_size << " ===\n";

    // --- make a random element set ---
    oc::PRNG prng(oc::toBlock(0xDEADBEEF, 0x01234567));
    std::vector<Element> elems = GetRandomSet(&prng, set_size);

    // --- choose a build seed (persisted inside rb_okvs) ---
    oc::block ro_seed = prng.get<oc::block>();

    // --- build the OKVS ---
    rb_okvs okvs(rb_okvs_size_table::get(set_size));
    okvs.build(elems, ro_seed);

    // quick sanity
    assert(!okvs.isEmpty());
    // assert(okvs.n == n_columns);

    ASE tmp = (ASE)okvs;
    rb_okvs okvs2 = rb_okvs(std::move(tmp));
    okvs2.setup(ro_seed);

    // --- verify every element evaluates to the encoded RHS ---
    for (const auto& e : elems)
    {
        oc::block want = random_oracle(e, ro_seed);
        oc::block got1 = okvs2.eval1(e);

        // eval() pushes into a vector
        BlockVec outs;
        okvs2.eval(e, outs);
        assert(!outs.empty());
        oc::block got2 = outs.back();

        if (got1 != want || got2 != want) {
            std::cerr << "Mismatch for element! "
                      << "want=" << want << " got1=" << got1 << " got2=" << got2 << "\n";
            assert(false && "rb_okvs evaluation mismatch");
        }        
    }

    std::cout << "OK: all " << set_size << " evaluations matched.\n";
}

int main()
{
    // A couple of sizes; you can tweak these depending on your build speed.
    // Heuristic: n should be comfortably larger than |set|; band_width in [16, 256].
    // const size_t set_size = 200;           // number of key/value pairs to encode
    // const size_t bw1      = 64;            // local band width per row
    // const size_t n2       = 4096;
    // const size_t bw2      = 128;

    for (int i = 7; i <= 22; ++i) 
        one_run(1 << i);

    std::cout << "\nAll rb_okvs tests passed\n";
    return 0;
}
