// 故意失败：只声明，不提供定义。
// 编译能通过，链接会报 undefined reference。
int missing_function();

int main() {
    return missing_function();
}
