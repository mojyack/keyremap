#pragma once
#include "keycodes.hpp"
#include "types.hpp"
#include "util/charconv.hpp"
#include "util/misc.hpp"

struct ConfigFile {
    std::string              device_name = "Keyremap Virtual Device";
    std::vector<std::string> captures;
    std::vector<MapConfig>   maps;
};

inline auto load_config(const std::string_view path) -> ConfigFile {
    auto config   = ConfigFile();
    auto file     = std::fstream(path);
    auto line     = std::string();
    auto line_num = size_t(0);
    while(std::getline(file, line)) {
        line_num += 1;
        if(line.empty() || line[0] == '#') {
            continue;
        }
        const auto elms = split_args(line);
        if(elms[0] == "device") {
            if(elms.size() != 2) {
                print(line_num, ": invalid line");
                continue;
            }
            config.device_name = elms[1];
        } else if(elms[0] == "capture") {
            if(elms.size() != 2) {
                print(line_num, ": invalid line");
                continue;
            }
            config.captures.emplace_back(elms[1]);
        } else if(elms[0] == "map") {
            if(elms.size() != 4) {
                print(line_num, ": invalid input");
                continue;
            }
            auto map = MapConfig();
            if(const auto dev = from_chars<int>(elms[1]); !dev) {
                print(line_num, ": invalid input");
                continue;
            } else {
                map.device = dev.value();
            }
            map.key.from = str2code(elms[2]);
            map.key.to   = str2code(elms[3]);
            config.maps.emplace_back(map);
        }
    }

    return config;
}

