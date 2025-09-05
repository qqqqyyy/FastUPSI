#ifndef Adaptive_H
#define Adaptive_H

#include "ASE/ASE.h"
#include "ASE/plain_ASE.h"
#include "ASE/poly.h"
#include "rbokvs/rb_okvs.h"

namespace upsi {

template<typename BaseType>
class Adaptive : public ASE
{
    static_assert(std::is_base_of<ASE, BaseType>::value, "BaseType must derive from ASE");


    public:
        std::vector<std::shared_ptr<BaseType> > nodes;
        //TODO: hash

        size_t start_size;
        size_t node_cnt;

        Adaptive(size_t _start_size) {
            start_size = _start_size;
            elem_cnt = 0;
            node_cnt = 0;
            nodes.push_back(std::make_shared<BaseType>(start_size));
        }

        void addASE();

        std::vector<std::shared_ptr<ASE> > insert(const std::vector<Element>& elem, oc::PRNG* prng = nullptr) override;
        void replaceASEs(
            int new_elem_cnt,
            const std::vector<std::shared_ptr<ASE> >& new_ASEs
        );
		void eval(Element elem, BlockVec& values) override;
};

}      // namespace upsi

#endif
