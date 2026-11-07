#pragma once

// namespace 作用域的 const 默认内部链接：每个包含本头文件的 TU 各有一份。
const int kShared = 7;

// 只声明，不定义；定义放在 const_linkage_def.cpp 里，只出现一次。
extern const int kExternal;

// C++17 inline variable：外部链接，但允许多个 TU 有相同定义，链接器合并。
inline constexpr int kInline = 7;

int shared_a();
int shared_b();
int external_a();
int external_b();
const int* shared_a_addr();
const int* shared_b_addr();
const int* inline_a_addr();
const int* inline_b_addr();
