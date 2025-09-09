set(TREE_SRCS
    "utils.cpp"
    "ASE/ASE.h"
    "ASE/plain_ASE.cpp"
    "ASE/poly.cpp"
    "rbokvs/rb_okvs.cpp"
    "network.h"
    "vole.cpp"
    "oprf.cpp"
    "binary_tree.cpp"
    "tree.cpp"
    "adaptive.cpp"
    "party.h"
    "tree_party.cpp"
)

add_library(upsi STATIC ${TREE_SRCS})
target_link_libraries(upsi oc::libOTe)