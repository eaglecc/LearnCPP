#pragma once

// 故意失败：带初始化器的 extern const 是定义，不是声明。
// 两个 TU 都包含本头文件时，会各生成一份 kExternalBad 定义。
extern const int kExternalBad = 7;
