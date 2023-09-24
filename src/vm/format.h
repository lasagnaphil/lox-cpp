#pragma once

#include "core/span.h"
#include "core/vector.h"
#include "vm/value.h"

bool formatted_print(FILE* f, std::string& error_msg, const char* format_str, Span<Value> args);
