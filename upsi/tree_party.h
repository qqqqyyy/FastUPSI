#include "party.h"
#include "crypto_tree.h"

namespace upsi{

class TreeParty : public Party{
    public: 
        CryptoTree<RawNode, RawNode> my_tree;
        CryptoTree<Poly, rb_okvs> other_tree;

        PRNG tree_prng;
        oc::block tree_seed;
        oc::block ro_seed;

        TreeParty(int _party, Socket* _chl, int _total_days, int _max_data_size):
                Party(_party, _chl, _total_days, _max_data_size){

            if(party == 0) {
                oc::block prng_seed = oc::sysRandomSeed();
                tree_seed = oc::sysRandomSeed();
                ro_seed = oc::sysRandomSeed();
                coproto::sync_wait(chl->send(prng_seed));
                coproto::sync_wait(chl->send(tree_seed));
                coproto::sync_wait(chl->send(ro_seed));
                coproto::sync_wait(chl->flush());
            }
            else {
                oc::block prng_seed;
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

        void sender(std::vector<Element> elems) { // query for elems

        }

        OPRFValueVec receiver() {

        }

        
        void my_addition(std::vector<Element> elems) {
            auto nodes = my_tree.insert(elems, &tree_prng);
            int cnt = nodes.size();

            Poly polys[cnt - 1];
            std::vector<Element> cur_elems;
            BlockVec cur_values;
            for (int i = 0; i < cnt - 1; ++i) {
                nodes[i].getElements(cur_elems);
                for (auto& x: cur_elems) {
                    cur_values.push_back(random_oracle(x, ro_seed));
                }
            }
            batchInterpolation(polys, cur_elems, cur_values);

            for (int i = 0; i < cnt - 1; ++i) {
                vole_receiver.get(polys[i].n);
                
            }
            
            rb_okvs stash; //TODO: size
            stash.build(nodes[cnt - 1].getElements(), ro_seed);


            
        }

        void other_addition() {
            
        }
        
};

}