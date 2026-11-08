// 故意编译失败：const 成员不能在构造函数体里赋值，只能在初始化列表里初始化。
class Config {
public:
    Config(int id, int& value) : value_(value) {
        id_ = id;  // 错误：id_ 是 const
    }

private:
    const int id_;
    int& value_;
};

int main() {
    int v = 0;
    Config cfg(1, v);
    return 0;
}
