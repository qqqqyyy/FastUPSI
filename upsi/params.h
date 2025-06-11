#pragma once

#include "upsi/utils.h"

namespace upsi {

struct PSIParams {

    // pointer to the running context object
    // Context* ctx;

    // number of days to run protocol for
    int total_days;

    // if creating mock trees, size of those trees
    int start_size;

    // parameters for the CryptoTrees
    int stash_size = DEFAULT_STASH_SIZE;
    int node_size = DEFAULT_NODE_SIZE;

    // addition only param set
    PSIParams(
        // Context* ctx,
        int total_days = 1,
        int start_size = 0
    ) : total_days(total_days), start_size(start_size){}

};

}
