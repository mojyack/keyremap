
#include <sys/ioctl.h>
#include <unistd.h>

#include "config.hpp"
#include "logger.hpp"
#include "uinput.hpp"

auto input_watcher_main(const int fd, const int vfd, const std::vector<KeyMap>& maps) -> void {
    auto event = input_event();
loop:
    if(read(fd, &event, sizeof(event)) != sizeof(event)) {
        warn("read() failed: ", errno);
        return;
    }

    if(event.type == EV_KEY) {
        for(auto& map : maps) {
            if(event.code == map.from) {
                event.code = map.to;
                break;
            }
        }
    }

    if(write(vfd, &event, sizeof(event)) != sizeof(event)) {
        warn("write() failed: ", errno);
        return;
    }
    goto loop;
}

auto run(const ConfigFile config) -> int {
    auto devices = std::vector<Device>();
    for(const auto& d : enumerate_devices()) {
        for(auto citer = size_t(0); citer < config.captures.size(); citer += 1) {
            if(config.captures[citer] == d.name) {
                auto& dev = devices.emplace_back(Device{.fd = unwrap(open_uinput_device(d.file_path.data()))});
                for(auto& m : config.maps) {
                    if(size_t(m.device) == citer) {
                        dev.maps.push_back(m.key);
                    }
                }
                break;
            }
        }
    }
    const auto vdev = unwrap(create_virtual_device(devices, config.device_name.data()));
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
    keyremap print-events {/dev/input/event*}   : show event log of the device
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
    } else if(command == "print-events") {
        if(argc != 3) {
            warn("invalid usage");
            print(usage);
            return 1;
        }
        return logger(argv[2]);
    } else if(command == "help") {
        print(usage);
        return 1;
    }

    warn("invalid usage");
    print(usage);
    return 1;
}
