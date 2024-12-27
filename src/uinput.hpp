#pragma once
#include <span>
#include <string>
#include <vector>

#include "util/fd.hpp"

struct InputDeviceInfo {
    std::string file_path;
    std::string name;
};

auto enumerate_devices() -> std::vector<InputDeviceInfo>;
auto open_uinput_device(const char* path, bool grab = true) -> std::optional<FileDescriptor>;
auto configure_virtual_device(std::span<int> parent_device_fds) -> std::optional<FileDescriptor>;
auto create_virtual_device(int fd, const char* device_name) -> bool;
