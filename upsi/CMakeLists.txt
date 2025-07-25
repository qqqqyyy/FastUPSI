set(TREE_SRCS
    "utils.cpp"
    "ASE/ASE.h"
    "ASE/plain_ASE.cpp"
    "ASE/poly.cpp"
    "ASE/cuckoo.cpp"
    "vole.cpp"
    "binary_tree.cpp"
    "party.h"
    "tree_party.h"
)

add_library(upsi STATIC ${TREE_SRCS})
target_link_libraries(upsi oc::libOTe)