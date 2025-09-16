#include "binary_tree.h"

// #include <iomanip>


namespace upsi {

////////////////////////////////////////////////////////////////////////////////
// GENERIC TREE METHODS
////////////////////////////////////////////////////////////////////////////////

template<typename NodeType>
void BinaryTree<NodeType>::setup(oc::PRNG* prng, oc::block seed, size_t node_size) {
    this->node_size = node_size;
	this->seed = seed;
	this->prng = prng;
	this->depth = 0;
	this->elem_cnt = 0;
	this->n = 0;

	addNode(); //empty node
    addNode(); //root;
}

template<typename NodeType>
inline void BinaryTree<NodeType>::addNode() {
	// auto cur_node = std::make_shared<NodeType>(node_size);
	auto cur_node = NodeType(node_size);
    this->nodes.push_back(cur_node);
	// for (int i = 0; i < node_size; ++i) ase.push_back(cur_node->ase[i]);
	// ase.insert(ase.end(), cur_node->ase.begin(), cur_node->ase.end());
	n += cur_node.n;
}

/// @brief Helper methods
template<typename NodeType>
void BinaryTree<NodeType>::addNewLayer() {
    this->depth += 1;
    size_t new_size = (1 << (this->depth + 1));
	// std::cout << new_size << "\n";
    while (this->nodes.size() < new_size) {
		addNode();
		// if(new_size == 4) std::cout << this->nodes.size() << "\n";
	}
}

// compute leaf index of a binary hash
template<typename NodeType>
int BinaryTree<NodeType>::computeIndex(BinaryHash binary_hash) {//TODO
	int x = 1;
	for (u64 i = 0; i < this->depth; ++i) {
        if (binary_hash[i] == false) x = (x << 1);
        else if (binary_hash[i] == true) x = ((x << 1) | 1);
    }
    return x;
}

// Return indices in paths in decreasing order (including stash)
template<typename NodeType>
void BinaryTree<NodeType>::extractPathIndices(int* leaf_ind, int leaf_cnt, std::vector<int> &ind) {
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
template<typename NodeType>
int* BinaryTree<NodeType>::generateRandomPaths(size_t cnt, std::vector<int> &ind, std::vector<BinaryHash> &hsh) { //ind: node indices

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
template<>
std::pair<std::vector<PlainASE* >, std::vector<int> > BinaryTree<PlainASE>::insert(const std::vector<Element> &elem, PlainASE &stash) {
	int new_elem_cnt = elem.size();

	// add new layer when tree is full
	// oc::Timer t1("New Layer");
	// t1.setTimePoint("begin");
	while(new_elem_cnt + this->elem_cnt >= (1 << (this->depth + 1))) addNewLayer();
	// t1.setTimePoint("end");
	// std::cout << t1 << "\n";
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
		for (int u = leaf_ind[o]; u; u >>= 1) {
			BlockVec cur;
			nodes[u].getElements(cur);
			int cur_size = cur.size();

			for (u64 i = 0; i < cur_size; ++i) {
				int x = computeIndex(computeBinaryHash(cur[i], seed));
				int steps = 0;
				if(x != leaf_ind[o]) steps = 32 - __builtin_clz(x ^ leaf_ind[o]);
				tmp_elem[steps].push_back(cur[i]);
			}

			nodes[u].clear();
		}

		BlockVec cur;
		stash.getElements(cur);
		cur.push_back(elem[o]);
		int cur_size = cur.size();
		for (u64 i = 0; i < cur_size; ++i) {
			int x = computeIndex(computeBinaryHash(cur[i], seed));
			int steps = 0;
			if(x != leaf_ind[o]) steps = 32 - __builtin_clz(x ^ leaf_ind[o]);
			tmp_elem[steps].push_back(cur[i]);
		}
		stash.clear();
		

		//fill the path
		int st = 0, steps = 0;
		for (int u = leaf_ind[o]; u; u >>= 1, ++steps) {
			while(st <= steps && tmp_elem[st].empty()) ++st;
			while(st <= steps) {
				Element cur_elem = tmp_elem[st].back();
				if(nodes[u].insertElement(cur_elem)) tmp_elem[st].pop_back();
				else break;
				while(st <= steps && tmp_elem[st].empty()) ++st;
			}
		}

		while(st <= steps && tmp_elem[st].empty()) ++st;
		while(st <= steps) {
			Element cur_elem = tmp_elem[st].back();
			if(stash.insertElement(cur_elem)) tmp_elem[st].pop_back();
			else throw std::runtime_error("stash full");
			while(st <= steps && tmp_elem[st].empty()) ++st;
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
	std::vector<PlainASE* > rs;
	for (u64 i = 0; i < node_cnt; ++i) {
        rs.push_back(&(nodes[ind[i]]));
    }
	return std::make_pair(rs, ind);
}

template<>
std::pair<std::vector<Poly* >, std::vector<int> > BinaryTree<Poly>::insert(const std::vector<Element> &elem, PlainASE &stash) {
	int new_elem_cnt = elem.size();
	while(new_elem_cnt + this->elem_cnt >= (1 << (this->depth + 1))) {
		// std::cout << this->depth << "\n";
		addNewLayer();
	}
	this->elem_cnt += new_elem_cnt;
	return std::make_pair(std::vector<Poly* >(), std::vector<int>());
}

// Update tree (receiver)
template<typename NodeType>
std::vector<int> BinaryTree<NodeType>::update(int new_elem_cnt) {

	// int node_cnt = new_nodes.size();

	// add new layer when tree is full
	// oc::Timer t1("New Layer");
	// t1.setTimePoint("begin");
	while(new_elem_cnt + this->elem_cnt >= (1 << (this->depth + 1))) addNewLayer();
	// t1.setTimePoint("end");
	// std::cout << t1 << "\n";
	//std::cerr << "new depth: " << this->depth << std::endl;

	std::vector<int> ind;
	// generate hash
	// std::cout << "[BinaryTree] update " << new_elem_cnt << std::endl;
	auto hsh = generateRandomHash(prng, new_elem_cnt);
	int *leaf_ind = generateRandomPaths(new_elem_cnt, ind, hsh);
	// std::cout << "[BinaryTree] update done.\n\n";
	delete [] leaf_ind;

	// assert(node_cnt == ind.size());

	//for (u64 i = 0; i < node_cnt; ++i) std::cerr << ind[i] << std::endl;

	// replace nodes (including stash)
	// for (u64 i = 0; i < node_cnt; ++i) {
	// 	assert(nodes[ind[i]]->ase.size() == new_nodes[i]->ase.size());
	// 	nodes[ind[i]]->copy(*(new_nodes[i]));
	// }
	// // update elem_cnt
	this->elem_cnt += new_elem_cnt;

	return ind;
}

template<>
void BinaryTree<Poly>::eval_oprf(Element elem, oc::block delta, oc::block ro_seed, OPRFValueVec& values) {
    //std::cerr << "computing binary hash of "<< element << "\n";
    BinaryHash binary_hash = computeBinaryHash(elem, seed);
    //std::cerr << "hash is " << binary_hash << "\n";
    //std::cerr << "computing index...\n";
    int leaf_index = computeIndex(binary_hash);
    //std::cerr << "get a path from " << leaf_index << std::endl;

	//std::cerr << "tree size = " << nodes.size() << std::endl;
	OPRF<Poly> oprf_poly;
	for (int u = leaf_index; u; u >>= 1) {
		values.push_back(oprf_poly.sender(elem, u, nodes[u], delta, ro_seed));
	}
}

template<>
void BinaryTree<PlainASE>::eval_oprf(Element elem, oc::block delta, oc::block ro_seed, OPRFValueVec& values) {
	throw std::runtime_error("eval for PlainASE binary tree");
}


template<>  
int BinaryTree<PlainASE>::find(const Element& elem, bool remove) {
    BinaryHash binary_hash = computeBinaryHash(elem, seed);
    int leaf_index = computeIndex(binary_hash);
	for (int u = leaf_index; u; u >>= 1) if(nodes[u].find(elem, remove)) return u;
	throw std::runtime_error("binary tree: element not found");
	return 0;
}


template<>  
int BinaryTree<Poly>::find(const Element& elem, bool remove) {
	throw std::runtime_error("find() for Poly binary tree");
}


template class BinaryTree<PlainASE>;
template class BinaryTree<Poly>;

} // namespace upsi
