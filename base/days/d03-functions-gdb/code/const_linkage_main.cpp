#include "const_linkage.h"

#include <iostream>

int main() {
    std::cout << "kShared: a=" << shared_a() << ", b=" << shared_b() << '\n';
    std::cout << "kExternal: a=" << external_a() << ", b=" << external_b() << '\n';
    std::cout << "kInline: " << kInline << '\n';

    std::cout << "a kShared addr=" << static_cast<const void*>(shared_a_addr())
              << ", b kShared addr=" << static_cast<const void*>(shared_b_addr())
              << '\n';
    std::cout << "a kInline addr=" << static_cast<const void*>(inline_a_addr())
              << ", b kInline addr=" << static_cast<const void*>(inline_b_addr())
              << '\n';

    std::cout << "kShared same address? "
              << (shared_a_addr() == shared_b_addr() ? "true" : "false") << '\n';
    std::cout << "kInline same address? "
              << (inline_a_addr() == inline_b_addr() ? "true" : "false") << '\n';

    return 0;
}
