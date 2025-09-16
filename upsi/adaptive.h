#ifndef Adaptive_H
#define Adaptive_H

#include "ASE/ASE.h"
#include "ASE/plain_ASE.h"
#include "ASE/poly.h"
#include "rbokvs/rb_okvs.h"
#include "oprf.h"

namespace upsi {

template<typename BaseType>
class Adaptive : public ASE
{
    static_assert(std::is_base_of<ASE, BaseType>::value, "Adaptive: BaseType must derive from ASE");


    public:
        std::vector<std::shared_ptr<BaseType> > nodes;
        BlockVec seeds;
        //TODO: hash

        size_t start_size;
        size_t node_cnt;

        void setup(size_t _start_size = DEFAULT_ADAPTIVE_SIZE) {
            start_size = _start_size;
            elem_cnt = 0;
            node_cnt = 0;
            std::shared_ptr<BaseType> cur_node;
            if constexpr (std::is_same_v<BaseType, rb_okvs>) {
                cur_node = std::make_shared<rb_okvs>(rb_okvs_size_table::get(start_size));
            }
            else cur_node = std::make_shared<BaseType>(start_size);
            nodes.push_back(cur_node);
            seeds.push_back(oc::sysRandomSeed());
            n += cur_node->n;
            // for (int i = 0; i < cur_node->n; ++i) ase.push_back(cur_node->ase[i]);
        }

        void addASE();

        std::pair<std::vector<std::shared_ptr<BaseType> >, std::vector<int> > insert(
            const std::vector<Element>& elem, 
            BlockVec& new_seeds
        );

        std::vector<int> update(int new_elem_cnt);

        // void replaceASEs(
        //     int new_elem_cnt,
        //     const BlockVec& new_seeds,
        //     const std::vector<std::shared_ptr<ASE> >& new_ASEs
        // );
        void eval_oprf(Element elem, oc::block delta, OPRFValueVec& values);

        oc::block& operator [] (const size_t& idx) override{
            int tmp = idx;
            for (int i = 0; i <= node_cnt; ++i) {
                if(tmp <= nodes[i]->n) return (*nodes[i])[tmp];
                tmp -= nodes[i]->n;
            }
            throw std::runtime_error("adaptive [] index out of range");
            return (*nodes[0])[0];
        }

        const oc::block& operator [] (const size_t& idx) const override{
            int tmp = idx;
            for (int i = 0; i <= node_cnt; ++i) {
                if(tmp <= nodes[i]->n) return (*nodes[i])[tmp];
                tmp -= nodes[i]->n;
            }
            throw std::runtime_error("adaptive [] index out of range");
            return (*nodes[0])[0];
        }
};

}      // namespace upsi

#endif
