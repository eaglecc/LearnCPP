// 故意失败：两个调用都没有唯一最优候选。
// 不要加入 CMake，只用来记录编译错误。

void rank(long);
void rank(double);

void h(int);
void h(int, int = 0);

int main() {
    rank(1);  // int -> long 和 int -> double 都是 conversion
    h(1);     // h(int) 与 h(int, int = 0) 都能调用
    return 0;
}
