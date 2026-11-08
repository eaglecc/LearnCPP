// 故意编译失败：拷贝被 = delete 的类。
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

int main() {
    NonCopyable a;
    NonCopyable b = a;  // 错误：拷贝构造被 delete
    NonCopyable c;
    c = a;              // 错误：拷贝赋值被 delete
    return 0;
}
