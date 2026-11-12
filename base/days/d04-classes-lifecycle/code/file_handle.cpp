#include "file_handle.h"

FileHandle::FileHandle(const char* path, const char* mode) : fp_(std::fopen(path, mode)) {}

FileHandle::~FileHandle() {
    if (fp_) {
        std::fclose(fp_);
    }
}

bool FileHandle::is_open() const {
    return fp_ != nullptr;
}

std::FILE* FileHandle::get() const {
    return fp_;
}

bool FileHandle::write(const char* text) {
    if (!fp_) {
        return false;
    }
    return std::fputs(text, fp_) != EOF;
}