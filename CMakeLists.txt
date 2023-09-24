cmake_minimum_required(VERSION 3.24)
project(lox)

set(CMAKE_CXX_STANDARD 17)

set(LOX_SRCS
        src/core/log.cpp
        src/core/file.cpp
        src/vm/vm.cpp
        src/vm/opcode.cpp
        src/vm/chunk.cpp
        src/vm/compiler.cpp
        src/vm/value.cpp
        src/vm/string.cpp
        src/vm/array.cpp
        src/vm/table.cpp
        src/vm/object.cpp
        src/vm/string_interner.cpp
        src/main.cpp
)

set(LOX_FMT_SRCS
        src/vm/format.cpp
        deps/fmt/src/format.cc
)

# For the lox-fmt library (which need to catch exceptions), compile with exceptions and RTTI ON.
add_library(lox-fmt STATIC ${LOX_FMT_SRCS})
target_include_directories(lox-fmt PRIVATE deps/fmt/include)
target_include_directories(lox-fmt PRIVATE src)
target_compile_options(lox-fmt PRIVATE /EHsc)

# For the fmt library we're actually using, compile with exceptions and RTTI OFF.
add_subdirectory(deps/fmt EXCLUDE_FROM_ALL)
target_compile_definitions(fmt PRIVATE FMT_EXCEPTIONS=0)
target_compile_options(fmt PRIVATE /D_HAS_EXCEPTIONS=0 /GR-)

# For lox, compile with exceptions and RTTI OFF.
add_executable(lox ${LOX_SRCS})
target_include_directories(lox PRIVATE src)
target_link_libraries(lox PRIVATE lox-fmt fmt)
target_compile_options(lox PRIVATE /D_HAS_EXCEPTIONS=0 /GR-)
