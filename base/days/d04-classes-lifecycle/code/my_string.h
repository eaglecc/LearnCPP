#pragma once
#include <cstddef>

class String {
public:
    String();
    explicit String(const char* str);
    String(const String& other);
    String& operator=(const String& other);
    ~String();

    const char* c_str() const;
    std::size_t size() const;
    bool empty() const;

private:
    char* data_;
    std::size_t size_;
};