#pragma once

// 故意失败：去掉 inline 后，函数定义放进头文件，
// 被多个 TU 包含时会生成多份外部链接定义。
int twice(int x) {
    return x * 2;
}
