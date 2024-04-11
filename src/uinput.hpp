#pragma once
#include <span>
#include <string>

#include "util/error.hpp"

struct InputDeviceInfo {
    std::string file_path;
    std::string name;
};

auto enumerate_devices() -> std::vector<InputDeviceInfo>;
auto open_uinput_device(const char* path, bool grab = true) -> Result<int>;
auto configure_virtual_device(std::span<int> parent_device_fds) -> Result<int>;
auto create_virtual_device(int fd, const char* device_name) -> void;
