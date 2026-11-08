// 编译失败实验：加 -DEXPLICIT 打开下面两行。
// 不加宏时只编译 Box a(10)，用来确认文件里没有别的错误。
class Box {
public:
    explicit Box(int size);
    int size() const;
};

void take(Box b);

int main() {
    Box a(10);
#ifdef EXPLICIT
    Box b = 10; // 错误：Box(int) 是 explicit 的，不能隐式转换
    take(10); // 错误：Box(int) 是 explicit 的，不能隐式转换
#endif
}