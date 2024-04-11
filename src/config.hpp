#pragma once
#include <string>
#include <vector>

struct KeyMap {
    int16_t from;
    int16_t to;
};

struct MapConfig {
    int32_t device;
    KeyMap  key;
};

struct ConfigFile {
    std::string              device_name = "Keyremap Virtual Device";
    std::vector<std::string> captures;
    std::vector<MapConfig>   maps;
};

auto load_config(std::string_view path) -> ConfigFile;
