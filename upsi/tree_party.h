#include "party.h"
#include "tree.h"
#include "oprf.h"

namespace upsi{

class TreeParty : public Party{
    public: 
        Tree<PlainASE, PlainASE> my_tree;
        Tree<Poly, rb_okvs> other_tree;

        oc::PRNG tree_prng;
        oc::block tree_seed;

        TreeParty(int _party, oc::Socket* _chl, int _total_days, int _max_data_size):
                Party(_party, _chl, _total_days, _max_data_size){

            oc::block prng_seed;
            if(party == 0) {
                prng_seed = oc::sysRandomSeed();
                tree_seed = oc::sysRandomSeed();
                ro_seed = oc::sysRandomSeed();
                coproto::sync_wait(chl->send(prng_seed));
                coproto::sync_wait(chl->send(tree_seed));
                coproto::sync_wait(chl->send(ro_seed));
                coproto::sync_wait(chl->flush());
            }
            else {
                coproto::sync_wait(chl->recv(prng_seed));
                coproto::sync_wait(chl->recv(tree_seed));
                coproto::sync_wait(chl->recv(ro_seed));
            }
            tree_prng.SetSeed(prng_seed);
            my_tree.setup(&tree_prng, tree_seed, DEFAULT_STASH_SIZE, DEFAULT_NODE_SIZE);
            other_tree.setup(&tree_prng, tree_seed, DEFAULT_STASH_SIZE, DEFAULT_NODE_SIZE); //TODO: okvs size
        }


        void setup(std::vector<Element> elems, const std::vector<Element>& cur_I) {
            if(party == 0) {
                my_addition(elems);
                other_addition();
            }
            else {
                other_addition();
                my_addition(elems);
            }
            intersection = cur_I;
        }

        void one_day(std::vector<Element> addition_set, std::vector<Element> deletion_set) {
            int cnt_del = deletion_part(deletion_set);
            int cnt_add = addition_part(addition_set);
        }

        int addition_part(std::vector<Element> addition_set);
        int deletion_part(std::vector<Element> deletion_set);

        void sender(std::vector<Element> elems); // query for elems

        std::vector<Element> receiver();
        
        void my_addition(std::vector<Element> elems);

        void other_addition();
        
};

}