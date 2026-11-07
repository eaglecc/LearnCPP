#include "const_linkage.h"

int shared_a() {
    return kShared;
}

int external_a() {
    return kExternal;
}

const int* shared_a_addr() {
    return &kShared;
}

const int* inline_a_addr() {
    return &kInline;
}
