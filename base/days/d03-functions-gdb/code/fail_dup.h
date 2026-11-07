#pragma once

// 故意失败：普通函数定义放在头文件里。
// 两个 TU 包含它，就会各自生成一份外部链接的 twice。
int twice(int x) {
    return x * 2;
}
