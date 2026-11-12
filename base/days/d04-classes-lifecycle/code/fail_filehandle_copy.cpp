// 故意失败：FileHandle 的拷贝构造 / 拷贝赋值被 = delete。
// 只用来记录编译错误，不进 CMake。
#include "file_handle.h"

int main() {
    FileHandle a("/tmp/d04_filehandle.txt");
    FileHandle b = a;   // 错误：拷贝构造被 delete
    FileHandle c("/tmp/d04_other.txt");
    c = a;              // 错误：拷贝赋值被 delete
}
