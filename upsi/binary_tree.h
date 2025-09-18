#ifndef BinaryTree_H
#define BinaryTree_H

#include "ASE/ASE.h"
#include "ASE/plain_ASE.h"
#include "ASE/poly.h"
#include "oprf.h"

namespace upsi {

template<typename NodeType>
class BinaryTree : public ASE
{
    static_assert(std::is_base_of<ASE, NodeType>::value, "NodeType must derive from ASE");

    protected:

        // The node and stash size of the tree
        int node_size;

        /// @brief Helper Methods
        int computeIndex(BinaryHash binary_hash);
        void extractPathIndices(int* leaf_ind, int leaf_cnt, std::vector<int> &ind);
        int* generateRandomPaths(int cnt, std::vector<int> &ind, std::vector<BinaryHash> &hsh);

    public:

        // Array list representation
        /*    0 (empty)
        	  1 (root)
           2     3
          4  5  6  7
        */
        std::vector<NodeType> nodes;
        oc::block seed;
        oc::PRNG* prng;

        // Depth of the tree (empty tree or just root is depth 0)
        int depth = 0;

        // setup before insert
        void setup(oc::PRNG* prng, oc::block seed = oc::ZeroBlock, int node_size = DEFAULT_NODE_SIZE);
        // void clear() override;

        inline void addNode();

        // Add a new layer to the tree, expand the size of the vector
        void addNewLayer();

        // insert is only used for plaintext tree
        std::pair<std::vector<NodeType* >, std::vector<int> > insert(const std::vector<Element> &elem, PlainASE& stash);
        std::vector<int> update(int new_elem_cnt);

        void eval_oprf(Element elem, oc::block delta, oc::block ro_seed, OPRFValueVec& values);
        
        int find(const Element& elem, bool remove = false);


        oc::block& operator [] (const size_t& idx) override{
            return nodes[idx / node_size][idx % node_size];
        }

        const oc::block& operator [] (const size_t& idx) const override{
            return nodes[idx / node_size][idx % node_size];
        }
};

}      // namespace upsi

#endif
