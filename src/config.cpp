#include <array>
#include <fstream>

#include <linux/uinput.h>

#include "config.hpp"
#include "macros/unwrap.hpp"
#include "util/charconv.hpp"
#include "util/split.hpp"

namespace {
auto process_line(ConfigFile& config, std::string_view line) -> bool {
    const auto elms = split(line, " ");
    if(elms[0] == "device_name") {
        ensure(elms.size() == 2);
        config.device_name = elms[1];
    } else if(elms[0] == "capture_device") {
        ensure(elms.size() == 2);
        config.captures.push_back(Capture{std::string(elms[1]), {}});
    } else if(elms[0] == "map") {
        ensure(elms.size() == 8);
        auto elms_int = std::array<uint16_t, 7>();
        for(auto i = 0; i < 7; i += 1) {
            unwrap(num, from_chars<int>(elms[i + 1]));
            elms_int[i] = num;
        }
        auto       map        = EventMap();
        const auto device     = elms_int[0];
        const auto match_type = elms_int[1];
        map.match_code        = elms_int[2];
        map.match_value       = elms_int[3];
        map.rewrite_code      = elms_int[4];
        map.rewrite_value     = elms_int[5];
        map.send_up           = elms_int[6];
        ensure(device < config.captures.size(), "no such capture device");
        ensure(match_type < EV_CNT, "type must be less than ", EV_CNT);
        auto& maps = config.captures[device].maps;
        if(maps.size() <= match_type) {
            maps.resize(match_type + 1);
        }
        maps[match_type].emplace_back(map);
    }
    return true;
}
} // namespace

auto ConfigFile::from_file(const char* const path) -> std::optional<ConfigFile> {
    auto config   = ConfigFile();
    auto file     = std::fstream(path);
    auto line     = std::string();
    auto line_num = size_t(0);
    while(std::getline(file, line)) {
        line_num += 1;
        if(line.empty() || line[0] == '#') {
            continue;
        }
        ensure(process_line(config, line), "error at line ", line_num);
    }

    return config;
}

