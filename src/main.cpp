#include "vm/chunk.h"
#include "vm/scanner.h"
#include "vm/vm.h"

int main(int argc, const char* argv[]) {
    VM vm;
    if (argc == 1) {
        vm.repl();
    }
    else if (argc == 2) {
        vm.run_file(argv[1]);
    }
    else {
        fprintf(stderr, "Usage: lox [path]\n");
        exit(64);
    }

    return 0;
}
