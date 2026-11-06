cmake_minimum_required(VERSION 3.28)
project(CPPLearn LANGUAGES CXX)

# 用 -std=c++20，而不是 -std=gnu++20。
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(d01-hello days/d01-toolchain/hello.cpp)

# D02：每个练习文件一个独立可执行 target。
add_executable(d02-types days/d02-basics/code/types.cpp)
target_compile_features(d02-types PRIVATE cxx_std_20)
target_compile_options(d02-types PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d02-pointers days/d02-basics/code/pointers.cpp)
target_compile_features(d02-pointers PRIVATE cxx_std_20)
target_compile_options(d02-pointers PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-funcs days/d03-functions-gdb/code/overload.cpp)
target_compile_features(d03-funcs PRIVATE cxx_std_20)
target_compile_options(d03-funcs PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-defaults days/d03-functions-gdb/code/default_args.cpp)
target_compile_features(d03-defaults PRIVATE cxx_std_20)
target_compile_options(d03-defaults PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)