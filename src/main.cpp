#include <thread>

#include <linux/uinput.h>
#include <unistd.h>

#include "config.hpp"
#include "uinput.hpp"

struct Device {
    int                                fd;
    std::vector<std::vector<EventMap>> maps;
    std::thread                        worker;
};

auto input_watcher_main(const int fd, const int vfd, const std::vector<std::vector<EventMap>>& maps) -> void {
    auto event = input_event();
loop:
    if(read(fd, &event, sizeof(event)) != sizeof(event)) {
        warn("read() failed: ", errno);
        return;
    }

    if(event.type >= maps.size()) {
        goto passthrough;
    }

    for(auto& map : maps[event.type]) {
        if(event.code != map.match_code) {
            continue;
        }
        if(map.match_value != ignore_value && event.value != map.match_value) {
            continue;
        }
        event.type = EV_KEY;
        event.code = map.rewrite_code;
        if(map.rewrite_value != ignore_value) {
            event.value = map.rewrite_value;
        }
        if(write(vfd, &event, sizeof(event)) != sizeof(event)) {
            warn("write() failed: ", errno);
            return;
        }
        if(!map.send_up) {
            goto loop;
        }
        event.value = 0; // release
        goto passthrough;
    }

passthrough:
    if(write(vfd, &event, sizeof(event)) != sizeof(event)) {
        warn("write() failed: ", errno);
        return;
    }
    goto loop;
}

auto run(ConfigFile config) -> int {
    auto device_fds = std::vector<int>();
    auto devices    = std::vector<Device>();
    for(const auto& d : enumerate_devices()) {
        for(auto citer = size_t(0); citer < config.captures.size(); citer += 1) {
            auto& capture = config.captures[citer];
            if(capture.name == d.name) {
                const auto fd = open_uinput_device(d.file_path.data()).unwrap();
                device_fds.push_back(fd);
                devices.emplace_back(Device{.fd = fd, .maps = std::move(capture.maps)});
                break;
            }
        }
    }
    const auto vdev = create_virtual_device(device_fds, config.device_name.data()).unwrap();
    for(auto& dev : devices) {
        dev.worker = std::thread(input_watcher_main, dev.fd, vdev, dev.maps);
    }
    for(auto& dev : devices) {
        dev.worker.join();
    }
    return 0;
}

constexpr auto usage = R"(usage:
    keyremap list-devices                       : print available devices
    keyremap run {/path/to/config-file}         : run keyremap with given config file
    keyremap help                               : print this message
)";

auto main(const int argc, const char* argv[]) -> int {
    if(argc < 2) {
        print(usage);
        return 1;
    }
    const auto command = std::string_view(argv[1]);
    if(command == "list-devices") {
        for(const auto& d : enumerate_devices()) {
            print(d.name, ": ", d.file_path);
        }
        return 0;
    } else if(command == "run") {
        if(argc != 3) {
            warn("invalid usage");
            print(usage);
            return 1;
        }
        return run(load_config(argv[2]));
    } else if(command == "help") {
        print(usage);
        return 1;
    }

    warn("invalid usage");
    print(usage);
    return 1;
}
