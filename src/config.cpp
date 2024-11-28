#include <array>

#include <linux/uinput.h>

#include "config.hpp"
#include "util/charconv.hpp"
#include "util/misc.hpp"

auto load_config(const char* const path) -> ConfigFile {
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
        if(elms[0] == "device_name") {
            if(elms.size() != 2) {
                print(line_num, ": invalid line");
                goto next;
            }
            config.device_name = elms[1];
        } else if(elms[0] == "capture_device") {
            if(elms.size() != 2) {
                print(line_num, ": invalid line");
                goto next;
            }
            config.captures.push_back(Capture{std::string(elms[1]), {}});
        } else if(elms[0] == "map") {
            if(elms.size() != 8) {
                print(line_num, ": invalid input");
                goto next;
            }
            auto elms_int = std::array<uint16_t, 7>();
            for(auto i = 0; i < 7; i += 1) {
                if(const auto num = from_chars<int>(elms[i + 1])) {
                    elms_int[i] = num.value();
                } else {
                    print(line_num, ": invalid input");
                    goto next;
                }
            }
            auto       map        = EventMap();
            const auto device     = elms_int[0];
            const auto match_type = elms_int[1];
            map.match_code        = elms_int[2];
            map.match_value       = elms_int[3];
            map.rewrite_code      = elms_int[4];
            map.rewrite_value     = elms_int[5];
            map.send_up           = elms_int[6];
            if(config.captures.size() <= device) {
                print(line_num, ": no such capture device");
                goto next;
            }
            if(match_type >= EV_CNT) {
                print(line_num, ": type must be less than ", EV_CNT);
                goto next;
            }
            auto& maps = config.captures[device].maps;
            if(maps.size() <= match_type) {
                maps.resize(match_type + 1);
            }
            maps[match_type].emplace_back(map);
        }
    next:
        continue;
    }

    return config;
}

