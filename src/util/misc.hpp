#pragma once
#include <fstream>
#include <vector>

#include "error.hpp"

#ifdef CUTIL_NS
namespace CUTIL_NS {
#endif

template <class T = std::byte>
auto read_binary(auto path) -> Result<std::vector<T>> {
    auto ifs = std::ifstream(std::move(path));
    if(!ifs) {
        return Error("cannot open file");
    }

    ifs.seekg(0, std::ios_base::end);
    const auto cur = static_cast<size_t>(ifs.tellg());
    ifs.seekg(0, std::ios_base::beg);

    auto r = std::vector<T>((cur + sizeof(T) - 1) / sizeof(T));
    ifs.read(std::bit_cast<char*>(r.data()), cur);
    if(ifs.fail()) {
        return Error("failed to read file");
    }
    return r;
}

inline auto split(const std::string_view str, const std::string_view sep) -> std::vector<std::string_view> {
    auto ret = std::vector<std::string_view>();
    auto pos = std::string_view::size_type(0);
    while(true) {
        if(pos >= str.size()) {
            break;
        }
        const auto prev = pos;
        pos             = str.find(sep, pos);
        if(pos == std::string_view::npos) {
            if(prev != str.size()) {
                ret.emplace_back(str.substr(prev));
            }
            break;
        }

        ret.emplace_back(str.substr(prev, pos - prev));

        pos += sep.size();
    }
    return ret;
}

inline auto split_args(const std::string_view str) -> std::vector<std::string_view> {
    auto       result = std::vector<std::string_view>();
    const auto len    = str.size();
    auto       qot    = '\0';
    auto       arglen = size_t();
    for(auto i = size_t(0); i < len; i += 1) {
        while(i < len && std::isspace(str[i])) {
            i += 1;
        }
        if(i >= len) {
            break;
        }

        auto start = i;
        if(str[i] == '\"' || str[i] == '\'') {
            qot = str[i];
        }
        if(qot != '\0') {
            i += 1;
            start += 1;
            while(i < len && str[i] != qot) {
                i += 1;
            }
            if(i < len) {
                qot = '\0';
            }
            arglen = i - start;
        } else {
            while(i < len && !std::isspace(str[i])) {
                i += 1;
            }
            arglen = i - start;
        }
        result.emplace_back(str.data() + start, arglen);
    }
    // internal::dynamic_assert(qot == '\0', "unclosed quotes");
    return result;
}

#ifdef CUTIL_NS
}
#endif
