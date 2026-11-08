// 故意编译失败：= default 后面漏了分号。
class S {
public:
    S() = default
};

int main() {
    S s;
    static_cast<void>(s);
    return 0;
}
