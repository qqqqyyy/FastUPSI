#include "binary_tree.h"

// #include <iomanip>


namespace upsi {

////////////////////////////////////////////////////////////////////////////////
// GENERIC TREE METHODS
////////////////////////////////////////////////////////////////////////////////

template<typename NodeType, typename StashType>
BinaryTree<NodeType, StashType>::BinaryTree(size_t stash_size, size_t node_size) {
    this->node_size = node_size;
    this->stash_size = stash_size;

    // Index for root node is 1, index for stash node is 0
    // depth = 0
	auto stash = std::make_shared<StashType>(stash_size);
    this->nodes.push_back(stash);
	for (int i = 0; i < stash_size; ++i) ase.push_back(stash->ase[i]);
    addNode(); //root;
}

template<typename NodeType, typename StashType>
void BinaryTree<NodeType, StashType>::addNode() {
	auto cur_node = std::make_shared<NodeType>(node_size);
    this->nodes.push_back(cur_node);
	for (int i = 0; i < node_size; ++i) ase.push_back(cur_node->ase[i]);
}

/// @brief Helper methods
template<typename NodeType, typename StashType>
void BinaryTree<NodeType, StashType>::addNewLayer() {
    this->depth += 1;
    size_t new_size = (1 << (this->depth + 1));

    while (this->nodes.size() < new_size) addNode();
}

// compute leaf index of a binary hash
template<typename NodeType, typename StashType>
int BinaryTree<NodeType, StashType>::computeIndex(BinaryHash binary_hash) {//TODO
	int x = 1;
	for (u64 i = 0; i < this->depth; ++i) {
        if (binary_hash[i] == '0') x = (x << 1);
        else if (binary_hash[i] == '1') x = ((x << 1) | 1);
    }
    return x;
}

// Return indices in paths in decreasing order (including stash)
template<typename NodeType, typename StashType>
void BinaryTree<NodeType, StashType>::extractPathIndices(int* leaf_ind, int leaf_cnt, std::vector<int> &ind) {
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
int* BinaryTree<NodeType, StashType>::generateRandomPaths(size_t cnt, std::vector<int> &ind, std::vector<BinaryHash> &hsh) { //ind: node indices

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
std::vector<std::shared_ptr<ASE> > BinaryTree<NodeType, StashType>::insert(const std::vector<Element> &elem, oc::PRNG* prng) {
	int new_elem_cnt = elem.size();

	// add new layer when tree is full
	while(new_elem_cnt + this->elem_cnt >= (1 << (this->depth + 1))) addNewLayer();
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
		BlockVec tmp_elem[this->depth + 2];

		//std::cerr << "************leaf ind = " << leaf_ind[o] << std::endl;
		for (int u = leaf_ind[o]; ; u >>= 1) {
			BlockVec cur;
			nodes[u]->getElements(cur);
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

			nodes[u]->clear();
			if(u == 0) break;
		}

		//fill the path
		int st = 0;
		for (int u = leaf_ind[o], steps = 0; ; u >>= 1, ++steps) {
			while(st <= steps && tmp_elem[st].empty()) ++st;
			while(st <= steps) {
				Element cur_elem = tmp_elem[st].back();
				if(nodes[u]->insertElement(cur_elem)) tmp_elem[st].pop_back();
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
	for (size_t i = 0; i < nodes.size(); ++i) {
		std::cerr << nodes[i].node.size() << " ";
	} std::cerr << std::endl;*/

	delete [] leaf_ind;

	// update elem_cnt
	this->elem_cnt += new_elem_cnt;

	int node_cnt = ind.size();
	std::vector<std::shared_ptr<ASE> > rs;
	for (u64 i = 0; i < node_cnt; ++i) {
        rs.push_back(nodes[ind[i]]);
    }
	return rs;
}

// Update tree (receiver)
template<typename NodeType, typename StashType>
void BinaryTree<NodeType, StashType>::replaceNodes(int new_elem_cnt, const std::vector<std::shared_ptr<ASE> > &new_nodes, std::vector<BinaryHash> &hsh) {

	int node_cnt = new_nodes.size();

	// add new layer when tree is full
	while(new_elem_cnt + this->elem_cnt >= (1 << (this->depth + 1))) addNewLayer();
	//std::cerr << "new depth: " << this->depth << std::endl;

	std::vector<int> ind;
	int *leaf_ind = generateRandomPaths(new_elem_cnt, ind, hsh);
	delete [] leaf_ind;

	assert(node_cnt == ind.size());

	//for (u64 i = 0; i < node_cnt; ++i) std::cerr << ind[i] << std::endl;

	// replace nodes (including stash)
	for (u64 i = 0; i < node_cnt; ++i) {
		assert(nodes[ind[i]]->ase.size() == new_nodes[i]->ase.size());
		nodes[ind[i]]->copy(*(new_nodes[i]));
	}
	// update elem_cnt
	this->elem_cnt += new_elem_cnt;
}


template<typename NodeType, typename StashType>
void BinaryTree<NodeType, StashType>::eval(Element elem, BlockVec& values) {
    //std::cerr << "computing binary hash of "<< element << "\n";
    BinaryHash binary_hash = computeBinaryHash(elem);
    //std::cerr << "hash is " << binary_hash << "\n";
    //std::cerr << "computing index...\n";
    int leaf_index = computeIndex(binary_hash);
    //std::cerr << "get a path from " << leaf_index << std::endl;

	//std::cerr << "tree size = " << nodes.size() << std::endl;
	for (int u = leaf_index; ; u >>= 1) {
		nodes[u]->eval(elem, values);
		if (u == 0) break;
	}
}

template class BinaryTree<PlainASE, PlainASE>;
template class BinaryTree<Poly, Cuckoo>;

} // namespace upsi
