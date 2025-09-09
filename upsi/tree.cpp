#include "tree.h"

// #include <iomanip>


namespace upsi {

////////////////////////////////////////////////////////////////////////////////
// GENERIC TREE METHODS
////////////////////////////////////////////////////////////////////////////////

template<typename NodeType, typename StashType>
void Tree<NodeType, StashType>::setup(oc::PRNG* prng, oc::block seed, size_t stash_size, size_t node_size) {
    this->node_size = node_size;
    this->stash_size = stash_size;

	binary_tree.setup(prng, seed, node_size);

	stash = StashType(stash_size);
}

template<>
std::pair<std::vector<std::shared_ptr<PlainASE> >, std::vector<int> > Tree<PlainASE, PlainASE>::insert(const std::vector<Element> &elem) {
	return binary_tree.insert(elem, stash);
}

template<>
std::pair<std::vector<std::shared_ptr<Poly> >, std::vector<int> > Tree<Poly, rb_okvs>::insert(const std::vector<Element> &elem) {
	throw std::runtime_error("insert for Tree<Poly, rb_okvs>");
}

// Update tree (receiver)
template<typename NodeType, typename StashType>
std::vector<int> Tree<NodeType, StashType>::update(int new_elem_cnt) {
	return binary_tree.update(new_elem_cnt);
}

template<>
void Tree<Poly, rb_okvs>::eval_oprf(Element elem, oc::block delta, oc::block ro_seed, OPRFValueVec& values) {

    binary_tree.eval_oprf(elem, delta, ro_seed, values);

	OPRF<rb_okvs> oprf_okvs;
	values.push_back(oprf_okvs.sender(elem, 0, stash, delta, ro_seed));
}

template<>
void Tree<PlainASE, PlainASE>::eval_oprf(Element elem, oc::block delta, oc::block ro_seed, OPRFValueVec& values) {
	throw std::runtime_error("eval for PlainASE tree");
}

template class Tree<PlainASE, PlainASE>;
template class Tree<Poly, rb_okvs>;

} // namespace upsi
