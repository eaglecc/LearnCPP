#pragma once

// 两个 TU 各自实现这些外部函数，内部各自持有 counter 和 helper。
int a_value();
int b_value();
void a_increment();
void b_increment();
