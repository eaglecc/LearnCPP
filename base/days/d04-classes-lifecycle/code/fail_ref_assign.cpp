// 故意编译失败：引用成员不能在构造函数体里“赋值绑定”。
// 引用必须在初始化列表里绑定；函数体里的 v_ = v 是“写穿到被引用的对象”，
// 而且 v_ 还没有绑定，所以先报 uninitialized reference member。
class RefOnly {
public:
    RefOnly(int& v) {
        v_ = v;  // 错误：v_ 未绑定
    }

private:
    int& v_;
};

int main() {
    int x = 1;
    RefOnly r(x);
    static_cast<void>(r);
    return 0;
}
