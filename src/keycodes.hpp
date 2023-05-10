#pragma once
#include <string_view>

inline auto str2code(const std::string_view str) -> int {
#include "str2code.txt"
    return 0;
}

inline auto code2str(const int code) -> const char* {
    switch(code) {
#include "code2str.txt"
    default:
        return "unknown";
    }
}
