cmake_minimum_required(VERSION 3.27)

project(CppObjModel)


file(GLOB_RECURSE srcs CONFIGURE_DEPENDS include/*.hpp src/*.cpp)

add_executable(CppObjModel_EXE ${srcs})

target_include_directories(CppObjModel_EXE PUBLIC include)