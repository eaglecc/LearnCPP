// 故意编译失败：声明了拷贝构造（即使 = delete）后，隐式默认构造不再生成。
class NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

int main() {
    NonCopyable a;  // 错误：没有可用的默认构造函数
    static_cast<void>(a);
    return 0;
}
