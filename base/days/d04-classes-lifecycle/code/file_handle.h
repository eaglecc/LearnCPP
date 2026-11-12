#pragma once
#include <cstdio>

class FileHandle {
public:
    explicit FileHandle(const char* path, const char* mode = "r");
    ~FileHandle();

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    bool is_open() const;
    std::FILE* get() const;
    bool write(const char* text);

private:
    std::FILE* fp_;
};