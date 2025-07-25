#ifndef CryptoTree_H
#define CryptoTree_H

#include "crypto_node.h"
#include "poly.h"
#include "cuckoo.h"

namespace upsi {

template<typename NodeType, typename StashType>
class CryptoTree : public ASE
{
    static_assert(std::is_base_of<CryptoNode, NodeType>::value, "NodeType must derive from CryptoNode");
    static_assert(std::is_base_of<CryptoNode, StashType>::value, "StashType must derive from CryptoNode");

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
        std::vector<std::shared_ptr<CryptoNode> > crypto_tree;

        // Depth of the tree (empty tree or just root is depth 0)
        int depth = 0;

        // the number of set elements in the tree (= size of set)
        int actual_size = 0;

        CryptoTree(size_t stash_size, size_t node_size);
        void addNode();
        std::vector<std::shared_ptr<CryptoNode> > insert(oc::PRNG* prng, std::vector<Element> &elem);
        void replaceNodes(
            int new_elem_cnt,
            std::vector<std::shared_ptr<CryptoNode> >& new_nodes,
            std::vector<BinaryHash>& hsh
        );
		void eval(Element elem, BlockVec& values) override;

        // Status Serialize(S* tree);

        // Status Deserialize(const S& tree, Context* ctx, ECGroup* group);

        virtual void Print() = 0;
};

}      // namespace upsi

#endif
