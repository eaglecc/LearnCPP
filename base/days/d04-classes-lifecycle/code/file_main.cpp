// 验证 RAII：函数提前 return 时，FileHandle 的析构仍然关闭文件并 flush。
#include <cstdio>
#include <iostream>

#include "file_handle.h"

namespace {
constexpr const char* kPath = "/tmp/d04_filehandle.txt";
}  // namespace

void write_then_return() {
    FileHandle f(kPath, "w");
    if (!f.is_open()) {
        std::cout << "打开失败\n";
        return;
    }
    f.write("first line\n");
    std::cout << "write_then_return: 提前 return\n";
    return;   // f 在这里析构 -> fclose -> flush
}

void write_second_line() {
    FileHandle f(kPath, "a");
    if (!f.is_open()) {
        std::cout << "追加打开失败\n";
        return;
    }
    f.write("second line\n");
    return;   // 同样在析构时关闭
}

void print_file() {
    FileHandle f(kPath, "r");
    if (!f.is_open()) {
        std::cout << "读取失败\n";
        return;
    }
    char buf[256];
    while (std::fgets(buf, static_cast<int>(sizeof buf), f.get()) != nullptr) {
        std::cout << "  " << buf;
    }
}

int main() {
    write_then_return();
    std::cout << "重新打开文件读取（能读到内容说明已 fclose/flush）：\n";
    print_file();

    write_second_line();
    std::cout << "追加一行后再次读取：\n";
    print_file();
}
