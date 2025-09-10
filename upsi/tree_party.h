#ifndef TREE_PARTY_H
#define TREE_PARTY_H

#include "party.h"
#include "tree.h"
#include "oprf.h"

namespace upsi{

class TreeParty : public Party{
    public: 
        Tree<PlainASE, PlainASE> my_tree;
        Tree<Poly, rb_okvs> other_tree;

        oc::PRNG tree_prng, other_tree_prng;
        oc::block tree_seed;

        TreeParty(int _party, oc::Socket* _chl, int _total_days, std::string fn):
                Party(_party, _chl, _total_days, fn){

            oc::block prng_seed, other_prng_seed;

            prng_seed = oc::sysRandomSeed();
            coproto::sync_wait(chl->send(prng_seed));
            coproto::sync_wait(chl->recv(other_prng_seed));

            if(party == 0) {
                tree_seed = oc::sysRandomSeed();
                ro_seed = oc::sysRandomSeed();
                coproto::sync_wait(chl->send(tree_seed));
                coproto::sync_wait(chl->send(ro_seed));
                coproto::sync_wait(chl->flush());
            }
            else {
                coproto::sync_wait(chl->recv(tree_seed));
                coproto::sync_wait(chl->recv(ro_seed));
            }
            tree_prng.SetSeed(prng_seed);
            other_tree_prng.SetSeed(other_prng_seed);
            my_tree.setup(&tree_prng, tree_seed, DEFAULT_STASH_SIZE, DEFAULT_NODE_SIZE);
            other_tree.setup(&other_tree_prng, tree_seed, rb_okvs_size_table::get(DEFAULT_STASH_SIZE), DEFAULT_NODE_SIZE);
        }


        std::vector<Element> query(const std::vector<Element>& elems) override; // query for elems
        
        void addition(const std::vector<Element>& elems) override;
        
};

}

#endif