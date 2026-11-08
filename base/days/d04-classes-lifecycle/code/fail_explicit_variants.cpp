// 故意编译失败：对照 explicit 构造函数能/不能用于哪些初始化写法。
// 前两行（直接初始化、直接列表初始化）合法；其余四行都会编译失败。
class Box {
public:
    explicit Box(int size) : size_(size) {}
    int size() const { return size_; }

private:
    int size_;
};

void take(Box b) {
    static_cast<void>(b);
}

Box make() {
    return 10;  // 错误：返回时隐式转换为 Box
}

int main() {
    Box a(10);    // OK：直接初始化
    Box b{10};    // OK：直接列表初始化
    Box c = 10;   // 错误：复制初始化
    Box d = {10}; // 错误：复制列表初始化
    take(10);     // 错误：按值传参需要隐式转换

    static_cast<void>(a);
    static_cast<void>(b);
    static_cast<void>(c);
    static_cast<void>(d);
    return 0;
}
