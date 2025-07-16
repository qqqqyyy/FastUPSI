set(TREE_SRCS
    "utils.cpp"
    "ASE.h"
    "tree/poly.cpp"
    "tree/crypto_node.cpp"
    "tree/crypto_tree.cpp"
)

add_library(tree STATIC ${TREE_SRCS})
target_link_libraries(tree oc::libOTe)