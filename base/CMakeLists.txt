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

# D03：每个带 main 的练习文件一个独立 target。
add_executable(d03-overload days/d03-functions-gdb/code/overload.cpp)
target_compile_features(d03-overload PRIVATE cxx_std_20)
target_compile_options(d03-overload PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-defaults days/d03-functions-gdb/code/default_args.cpp)
target_compile_features(d03-defaults PRIVATE cxx_std_20)
target_compile_options(d03-defaults PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-namespaces days/d03-functions-gdb/code/namespaces.cpp)
target_compile_features(d03-namespaces PRIVATE cxx_std_20)
target_compile_options(d03-namespaces PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-linkage
    days/d03-functions-gdb/code/link_main.cpp
    days/d03-functions-gdb/code/link_stats.cpp)
target_compile_features(d03-linkage PRIVATE cxx_std_20)
target_compile_options(d03-linkage PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-debug days/d03-functions-gdb/code/debug_me.cpp)
target_compile_features(d03-debug PRIVATE cxx_std_20)
target_compile_options(d03-debug PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror
    -g -O0)
