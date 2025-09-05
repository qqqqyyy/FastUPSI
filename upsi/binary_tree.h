#ifndef BinaryTree_H
#define BinaryTree_H

#include "ASE/ASE.h"
#include "ASE/plain_ASE.h"
#include "ASE/poly.h"
#include "rbokvs/rb_okvs.h"

namespace upsi {

template<typename NodeType, typename StashType>
class BinaryTree : public ASE
{
    static_assert(std::is_base_of<ASE, NodeType>::value, "NodeType must derive from ASE");
    static_assert(std::is_base_of<ASE, StashType>::value, "StashType must derive from ASE");

    protected:

        // The node and stash size of the tree
        size_t node_size;
        size_t stash_size;

        /// @brief Helper Methods
        // Add a new layer to the tree, expand the size of the vector
        void addNewLayer();
        int computeIndex(BinaryHash binary_hash);
        void extractPathIndices(int* leaf_ind, int leaf_cnt, std::vector<int> &ind);
        int* generateRandomPaths(size_t cnt, std::vector<int> &ind, std::vector<BinaryHash> &hsh);

    public:

        // Array list representation
        /*    0 (stash)
        	  1 (root)
           2     3
          4  5  6  7
        */
        std::vector<std::shared_ptr<ASE> > nodes;

        // Depth of the tree (empty tree or just root is depth 0)
        int depth = 0;

        BinaryTree(size_t stash_size, size_t node_size);
        void addNode();
        std::vector<std::shared_ptr<ASE> > insert(const std::vector<Element> &elem, oc::PRNG* prng = nullptr) override;
        void replaceNodes(
            int new_elem_cnt,
            const std::vector<std::shared_ptr<ASE> >& new_nodes,
            std::vector<BinaryHash>& hsh
        );
		void eval(Element elem, BlockVec& values) override;

        // Status Serialize(S* tree);

        // Status Deserialize(const S& tree, Context* ctx, ECGroup* group);

        // virtual void Print() = 0;
};

}      // namespace upsi

#endif
