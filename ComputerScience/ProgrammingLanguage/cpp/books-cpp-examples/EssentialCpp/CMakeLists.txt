cmake_minimum_required (VERSION 3.27)

project ("EssentialCpp")

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS include/*.hpp src/*.cpp)

# 将源代码添加到此项目的可执行文件。
add_executable (EssentialCpp ${srcs})

target_include_directories(EssentialCpp PUBLIC include)
target_compile_definitions(EssentialCpp PUBLIC EXAMPLE_NAME=p99)
