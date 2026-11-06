// 故意失败：默认参数必须从右往左。
// 不要加入 CMake，只用来记录编译错误。

void bad(int a = 1, int b);

int main() {
    return 0;
}
