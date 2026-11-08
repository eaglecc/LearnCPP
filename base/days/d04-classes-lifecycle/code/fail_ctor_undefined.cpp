// 故意链接失败：构造函数只声明、没有定义。
class X {
public:
    X();  // 声明之后没有给出定义
};

int main() {
    X x;  // 需要调用 X::X()，链接期找不到定义
    return 0;
}
