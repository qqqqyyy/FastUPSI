#ifndef CryptoNode_H
#define CryptoNode_H
#include "../utils.h"

namespace upsi {

class CryptoNode
{
    public:
        size_t node_size;

        CryptoNode(size_t _node_size) {node_size = _node_size;}
        virtual ~CryptoNode() = default;

        virtual void clear() = 0;

        // Extract elements
        virtual std::vector<Element> getElemenets() {throw std::runtime_error("Extracting elements not supported");}

        // Add an element to the node, return true if success, false if it's already full
        virtual bool addElement(Element &elem) {throw std::runtime_error("Adding elements not supported");}

        // pad with padding elements to the node_size
        virtual void pad(oc::PRNG* prng) {throw std::runtime_error("Padding not supported");}

        // TODO: query OPRF values for an element, push them into values
        virtual void query(Element &elem, std::vector<oc::block>& values) {throw std::runtime_error("Queries not supported");}

        //TODO: Serialize, Deserialize
};

class RawNode : public CryptoNode{
    public:
        std::vector<Element> elems;
        RawNode(size_t _node_size) : CryptoNode(_node_size) {}
        void clear() override {elems.clear();}
        std::vector<Element> getElemenets() override {return elems;}
        bool addElement(Element &elem) override;
        void pad(oc::PRNG* prng) override;
};

} // namespace upsi

#endif
