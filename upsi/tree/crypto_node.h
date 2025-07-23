#ifndef CryptoNode_H
#define CryptoNode_H
#include "../ASE.h"

namespace upsi {

class CryptoNode: public ASE
{
    public:
        size_t node_size;

        CryptoNode(size_t _node_size = DEFAULT_NODE_SIZE) {node_size = _node_size;}
        virtual ~CryptoNode() = default;

        virtual void clear() = 0;

        // Extract elements
        virtual BlockVec getElements() {throw std::runtime_error("Extracting elements not supported");}

        // Add an element to the node, return true if success, false if it's already full
        virtual bool addElement(const Element &elem) {throw std::runtime_error("Adding elements not supported");}

        // pad with padding elements to the node_size
        virtual void pad(oc::PRNG* prng) {throw std::runtime_error("Padding not supported");}

        //TODO: Serialize, Deserialize
};

class RawNode : public CryptoNode{
    public:
        int elem_cnt;
        RawNode(size_t _node_size) : CryptoNode(_node_size) {
            ase.reserve(_node_size);
            elem_cnt = 0;
            for (int i = 0; i < _node_size; ++i) 
                ase.push_back(std::make_shared<oc::block>(oc::ZeroBlock));
        }
        void clear() override {elem_cnt = 0;}
        BlockVec getElements() override;
        bool addElement(const Element &elem) override;
        void pad(oc::PRNG* prng) override;
};

} // namespace upsi

#endif
