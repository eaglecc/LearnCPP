#pragma once

// inline 函数允许在多个 TU 中出现相同定义，链接器会合并。
inline int twice(int x) {
    return x * 2;
}
