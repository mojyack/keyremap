#pragma once
#include <string>
#include <vector>

constexpr auto ignore_value = uint16_t(-1);

struct EventMap {
    uint16_t send_up : 1;
    uint16_t match_code : 15;
    uint16_t match_value;
    uint16_t rewrite_code;
    uint16_t rewrite_value;
    // I want to add a send_up field here, but do not want to increase the size of the structure.
    // so I use the first bit of match_code instead.
};

static_assert(sizeof(EventMap) == 8);

struct Capture {
    std::string                        name;
    std::vector<std::vector<EventMap>> maps;
};

struct ConfigFile {
    std::string          device_name = "Keyremap Virtual Device";
    std::vector<Capture> captures;
};

auto load_config(std::string_view path) -> ConfigFile;
