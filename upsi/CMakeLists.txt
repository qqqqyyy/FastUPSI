set(TREE_SRCS
    "utils.cpp"
    "ASE.h"
    "vole.cpp"
    "party.h"
    "tree/crypto_node.cpp"
    "tree/poly.cpp"
    "tree/cuckoo.cpp"
    "tree/crypto_tree.cpp"
    "tree/tree_party.h"
)

add_library(tree STATIC ${TREE_SRCS})
target_link_libraries(tree oc::libOTe)