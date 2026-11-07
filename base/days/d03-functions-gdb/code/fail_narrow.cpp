// 故意失败：列表初始化禁止窄化。
int main() {
    int x{3.14};
    return x;
}
