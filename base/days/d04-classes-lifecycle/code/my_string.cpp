#include "my_string.h"

#include <cstring>

namespace {

// 分配 size + 1 字节，复制 size 字节内容，并补结尾的 '\0'。
char* allocate_and_copy(const char* src, std::size_t size) {
    char* data = new char[size + 1];
    std::memcpy(data, src, size);
    data[size] = '\0';
    return data;
}

}  // namespace

String::String() : data_(new char[1]{'\0'}), size_(0) {}

String::String(const char* str)
    : data_(allocate_and_copy(str, std::strlen(str))),
      size_(std::strlen(str)) {}

String::String(const String& other)
    : data_(allocate_and_copy(other.data_, other.size_)),
      size_(other.size_) {}

String& String::operator=(const String& other) {
    if (this == &other) {
        return *this;
    }

    // 先分配并复制新内存，成功后再释放旧内存；反过来的话，
    // 一旦分配失败，data_ 已经被释放，对象会处于坏状态。
    char* new_data = allocate_and_copy(other.data_, other.size_);
    delete[] data_;
    data_ = new_data;
    size_ = other.size_;
    return *this;
}

String::~String() { delete[] data_; }

const char* String::c_str() const { return data_; }

std::size_t String::size() const { return size_; }

bool String::empty() const { return size_ == 0; }
