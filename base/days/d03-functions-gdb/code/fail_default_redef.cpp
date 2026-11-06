// 故意失败：同一个参数不能在声明和定义里重复指定默认值。
// 不要加入 CMake，只用来记录编译错误。

void f(int x = 1);

void f(int x = 1) {
}

int main() {
    return 0;
}
