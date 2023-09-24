#include "vm/format.h"

#include "fmt/args.h"
#include "fmt/format.h"

bool formatted_print(FILE *f, std::string &error_msg, const char *format_str, Span<Value> args) {
    fmt::dynamic_format_arg_store<fmt::format_context> store;
    for (Value arg : args) {
        if (arg.is_number()) store.push_back(arg.as_number());
        else if (arg.is_bool()) store.push_back(arg.as_bool());
        else store.push_back(arg.to_std_string());
    }
    try {
        fmt::vprint(f, format_str, store);
    } catch (fmt::format_error& e) {
        error_msg = e.what();
        return false;
    }
    return true;
}
