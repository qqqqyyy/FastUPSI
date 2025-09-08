#include "party.h"
#include "binary_tree.h"
#include "oprf.h"

namespace upsi{

class TreeParty : public Party{
    public: 
        BinaryTree<PlainASE, PlainASE> my_tree;
        BinaryTree<Poly, rb_okvs> other_tree;

        oc::PRNG tree_prng;
        oc::block tree_seed;
        oc::block ro_seed;

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
            my_tree.seed = other_tree.seed = tree_seed;
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

        void sender(std::vector<Element> elems); // query for elems

        OPRFValueVec receiver();
        
        void my_addition(std::vector<Element> elems);

        void other_addition();
        
};

}