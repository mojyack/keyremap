#pragma once
#include "types.hpp"

struct ConfigFile {
    std::string              device_name = "Keyremap Virtual Device";
    std::vector<std::string> captures;
    std::vector<MapConfig>   maps;
};

auto load_config(std::string_view path) -> ConfigFile;
