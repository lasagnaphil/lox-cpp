cmake_minimum_required(VERSION 3.24)
project(lox)

set(CMAKE_CXX_STANDARD 17)

set(LOX_SRCS
        src/core/log.cpp
        src/main.cpp)

add_subdirectory(deps/fmt EXCLUDE_FROM_ALL)
target_compile_definitions(fmt PRIVATE FMT_EXCEPTIONS=0)

add_subdirectory(deps/parallel-hashmap EXCLUDE_FROM_ALL)

add_executable(lox ${LOX_SRCS})
target_link_libraries(lox PRIVATE fmt phmap)
