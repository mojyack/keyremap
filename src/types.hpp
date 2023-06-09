#pragma once
#include <thread>
#include <vector>

struct InputDeviceInfo {
    std::string file_path;
    std::string name;
};

struct KeyMap {
    int16_t from;
    int16_t to;
};

struct MapConfig {
    int32_t device;
    KeyMap  key;
};

struct Device {
    int                 fd;
    std::vector<KeyMap> maps;
    std::thread         worker;
};
