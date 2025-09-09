#ifndef Tree_H
#define Tree_H

#include "rbokvs/rb_okvs.h"
#include "binary_tree.h"

namespace upsi {

template<typename NodeType, typename StashType>
class Tree
{
    static_assert(std::is_base_of<ASE, NodeType>::value, "NodeType must derive from ASE");
    static_assert(std::is_base_of<ASE, StashType>::value, "StashType must derive from ASE");

    public:
        
        size_t node_size;
        size_t stash_size;
        BinaryTree<NodeType> binary_tree;
        StashType stash = StashType(DEFAULT_STASH_SIZE);

        // setup before insert
        void setup(oc::PRNG* prng, oc::block seed = oc::ZeroBlock, size_t stash_size = DEFAULT_STASH_SIZE, size_t node_size = DEFAULT_NODE_SIZE);

        void addNode();
        // insert is only for plaintext tree
        std::pair<std::vector<std::shared_ptr<NodeType> >, std::vector<int> > insert(const std::vector<Element> &elem);
        std::vector<int> update(int new_elem_cnt);
        // build is only used for adaptive
        // void build(const std::vector<Element>& elems, oc::block ro_seed = oc::ZeroBlock, oc::PRNG* prng = nullptr) override;
		
        // eval is only used for sender
        void eval_oprf(Element elem, oc::block delta, oc::block ro_seed, OPRFValueVec& values);
};

} // namespace upsi

#endif
