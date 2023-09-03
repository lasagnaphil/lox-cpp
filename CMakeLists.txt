cmake_minimum_required(VERSION 3.24)
project(lox)

set(CMAKE_CXX_STANDARD 17)

set(LOX_SRCS
        src/core/log.cpp
        src/vm/vm.cpp
        src/vm/chunk.cpp
        src/vm/compiler.cpp
        src/vm/value.cpp
        src/vm/string.cpp
        src/vm/table.cpp
        src/vm/string_interner.cpp
        src/main.cpp
)

add_subdirectory(deps/fmt EXCLUDE_FROM_ALL)
target_compile_definitions(fmt PRIVATE FMT_EXCEPTIONS=0)

add_executable(lox ${LOX_SRCS})
target_include_directories(lox PRIVATE src)
target_link_libraries(lox PRIVATE fmt)
