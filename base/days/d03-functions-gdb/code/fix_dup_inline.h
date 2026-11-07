#pragma once

// 修复方式 2：保留在头文件里，但加上 inline。
inline int twice(int x) {
    return x * 2;
}
