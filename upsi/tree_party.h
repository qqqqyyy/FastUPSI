#include "../party.h"
#include "crypto_tree.h"

namespace upsi{

class TreeParty : public Party{
    public: 
        CryptoTree<RawNode, RawNode> my_tree;
        CryptoTree<Poly, Cuckoo> other_tree;
        
        
};

}