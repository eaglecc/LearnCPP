#include "const_linkage.h"

int shared_b() {
    return kShared;
}

int external_b() {
    return kExternal;
}

const int* shared_b_addr() {
    return &kShared;
}

const int* inline_b_addr() {
    return &kInline;
}
