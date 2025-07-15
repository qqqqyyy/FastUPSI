#include "crypto_tree.h"

#include <iomanip>


namespace upsi {

////////////////////////////////////////////////////////////////////////////////
// GENERIC TREE METHODS
////////////////////////////////////////////////////////////////////////////////

template<typename NodeType, typename StashType>
CryptoTree<NodeType, StashType>::CryptoTree(size_t stash_size, size_t node_size) {
    this->node_size = node_size;
    this->stash_size = stash_size;

    // Index for root node is 1, index for stash node is 0
    // depth = 0
    this->crypto_tree.push_back(std::make_shared<StashType>(stash_size));
    this->crypto_tree.push_back(std::make_shared<NodeType>(node_size));

}

/// @brief Helper methods
template<typename NodeType, typename StashType>
void CryptoTree<NodeType, StashType>::addNewLayer() {
    this->depth += 1;
    size_t new_size = (1 << (this->depth + 1));

    while (this->crypto_tree.size() < new_size) {
        this->crypto_tree.emplace_back(std::make_shared<NodeType>(node_size));
    }
}

// compute leaf index of a binary hash
template<typename NodeType, typename StashType>
int CryptoTree<NodeType, StashType>::computeIndex(BinaryHash binary_hash) {//TODO
	int x = 1;
	for (u64 i = 0; i < this->depth; ++i) {
        if (binary_hash[i] == '0') x = (x << 1);
        else if (binary_hash[i] == '1') x = ((x << 1) | 1);
    }
    return x;
}

// Return indices in paths in decreasing order (including stash)
template<typename NodeType, typename StashType>
void CryptoTree<NodeType, StashType>::extractPathIndices(int* leaf_ind, int leaf_cnt, std::vector<int> &ind) {
	assert(ind.size() == 0);

	// add the indicies of leaves
	for (u64 i = 0; i < leaf_cnt; ++i)
		ind.push_back(leaf_ind[i]);

	// erase duplicates and sort in decreasing order
	std::sort(ind.begin(), ind.end(), std::greater<int>());
	ind.erase(std::unique(ind.begin(), ind.end()), ind.end());

	int node_cnt = ind.size();
	for (u64 i = 0; i < node_cnt; ++i) {
		if(ind[i] == 0) break; // stash
		int tmp = (ind[i] >> 1); // find its parent
		assert(ind[node_cnt - 1] >= tmp);
		if(ind[node_cnt - 1] > tmp) {
			ind.push_back(tmp);
			++node_cnt;
		}
	}
}

// Generate random paths, return the indices of leaves and nodes(including stash)
template<typename NodeType, typename StashType>
int* CryptoTree<NodeType, StashType>::generateRandomPaths(size_t cnt, std::vector<int> &ind, std::vector<BinaryHash> &hsh) { //ind: node indices

	// compute leaf indices of the paths
	int *leaf_ind = new int[cnt];
	for (u64 i = 0; i < cnt; ++i) leaf_ind[i] = computeIndex(hsh[i]);

	// extract indices of nodes in these paths (including stash)
	extractPathIndices(leaf_ind, cnt, ind);

	return leaf_ind;
	// the sender requires indices of leaves if update one path at a time
	// need to delete leaf_ind outside this function
}

// @brief Real methods

// Insert new set elements (sender)
// Return vector of (plaintext) nodes
// stash: index = 0
template<typename NodeType, typename StashType>
std::vector<std::shared_ptr<CryptoNode> > CryptoTree<NodeType, StashType>::insert(oc::PRNG* prng, std::vector<Element> &elem) {
	int new_elem_cnt = elem.size();

	// add new layer when tree is full
	while(new_elem_cnt + this->actual_size >= (1 << (this->depth + 1))) addNewLayer();
	// no need to tell the receiver the new depth of tree?

	// get the node indices in random paths
	std::vector<int> ind;

	// generate hash
	auto hsh = generateRandomHash(prng, new_elem_cnt);
	int *leaf_ind = generateRandomPaths(new_elem_cnt, ind, hsh);

	/*
		To compute lca of x , y:
		let t be the leftmost 1 of (x xor y), steps = log2(t) + 1
		lca = x / 2t = x >> steps
	*/
	for (int o = 0; o < new_elem_cnt; ++o) {
		// extract all elements in the path and empty the origin node
		std::vector<Element> tmp_elem[this->depth + 2];

		//std::cerr << "************leaf ind = " << leaf_ind[o] << std::endl;
		for (int u = leaf_ind[o]; ; u >>= 1) {
			std::vector<Element> cur;
			if(u == 0) cur.push_back(elem[o]);

			int cur_size = cur.size();
			//std::cerr << "tmp_node size  = " << tmp_node_size << std::endl;

			for (u64 i = 0; i < cur_size; ++i) {
				int x = computeIndex(computeBinaryHash(cur[i]));
				//if(u == 0 && i == 0) std::cerr<<"index is " << x << std::endl;
				int steps = 0;
				if(x != leaf_ind[o]) steps = 32 - __builtin_clz(x ^ leaf_ind[o]);
				tmp_elem[steps].push_back(cur[i]);
				//std::cerr << "add " << x << " to " << (x >> steps) << std::endl;
			}

			crypto_tree[u]->clear();
			if(u == 0) break;
		}

		//fill the path
		int st = 0;
		for (int u = leaf_ind[o], steps = 0; ; u >>= 1, ++steps) {
			while(st <= steps && tmp_elem[st].empty()) ++st;
			while(st <= steps) {
				Element cur_elem = tmp_elem[st].back();
				if(crypto_tree[u]->addElement(cur_elem)) tmp_elem[st].pop_back();
				else break;
				while(st <= steps && tmp_elem[st].empty()) ++st;
			}
			if(u == 0) break;
		}

		assert(st > this->depth);
        for (auto i = 0; i < this->depth + 2; i++) {
            assert(tmp_elem[i].empty());
        }
    }

	/*
	for (size_t i = 0; i < crypto_tree.size(); ++i) {
		std::cerr << crypto_tree[i].node.size() << " ";
	} std::cerr << std::endl;*/

	delete [] leaf_ind;

	// update actual_size
	this->actual_size += new_elem_cnt;

	int node_cnt = ind.size();
	std::vector<std::shared_ptr<CryptoNode> > rs;
	for (u64 i = 0; i < node_cnt; ++i) {
        rs.push_back(crypto_tree[ind[i]]);
    }
	return rs;
}

// Update tree (receiver)
template<typename NodeType, typename StashType>
void CryptoTree<NodeType, StashType>::replaceNodes(int new_elem_cnt, std::vector<std::shared_ptr<CryptoNode> > &new_nodes, std::vector<BinaryHash> &hsh) {

	int node_cnt = new_nodes.size();

	// add new layer when tree is full
	while(new_elem_cnt + this->actual_size >= (1 << (this->depth + 1))) addNewLayer();
	//std::cerr << "new depth: " << this->depth << std::endl;

	std::vector<int> ind;
	int *leaf_ind = generateRandomPaths(new_elem_cnt, ind, hsh);
	delete [] leaf_ind;

	assert(node_cnt == ind.size());

	//for (u64 i = 0; i < node_cnt; ++i) std::cerr << ind[i] << std::endl;

	// replace nodes (including stash)
	for (u64 i = 0; i < node_cnt; ++i) crypto_tree[ind[i]] = new_nodes[i];

	// update actual_size
	this->actual_size += new_elem_cnt;
}

// Find path for an element (including stash) and extract all elements on the path
template<typename NodeType, typename StashType>
std::vector<oc::block> CryptoTree<NodeType, StashType>::getPath(Element elem) {
    std::vector<oc::block> values;
    //std::cerr << "computing binary hash of "<< element << "\n";
    BinaryHash binary_hash = computeBinaryHash(elem);
    //std::cerr << "hash is " << binary_hash << "\n";
    //std::cerr << "computing index...\n";
    int leaf_index = computeIndex(binary_hash);
    //std::cerr << "get a path from " << leaf_index << std::endl;

	//std::cerr << "tree size = " << crypto_tree.size() << std::endl;
	for (int u = leaf_index; ; u >>= 1) {
		crypto_tree[u]->query(elem, values);
		if (u == 0) break;
	}
    return values;
}

template class CryptoTree<RawNode, RawNode>;

} // namespace upsi
