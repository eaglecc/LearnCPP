// 观察 -Wconversion 对隐式 double -> int 的警告。
// 正常构建 types.cpp 时不编译本文件（它有警告是故意的）。
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c warn_conversion.cpp

int main() {
    double pi = 3.14;
    int n = pi;  // 隐式转换：小数部分被丢弃
    return n;
}
