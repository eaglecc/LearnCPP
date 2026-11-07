#include "fix_dup_decl.h"

int use_twice(int x) {
    return twice(x) + 1;
}
