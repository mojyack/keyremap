#include <filesystem>

#include <fcntl.h>
#include <linux/uinput.h>

#include "uinput.hpp"
#include "util/error.hpp"

namespace {
template <size_t bits>
constexpr auto bits_to_bytes = (bits - 1) / 8 + 1;

template <int event, size_t max>
auto get_uinput_device_event_bits(const int fd) -> Result<std::array<std::byte, bits_to_bytes<max>>> {
    auto result = std::array<std::byte, bits_to_bytes<max>>();
    if(ioctl(fd, EVIOCGBIT(event, result.size()), result.data()) != result.size()) {
        return Error(build_string("ioctl() failed: ", errno));
    }
    return result;
}

template <size_t size>
auto check_bit(const std::array<std::byte, size>& data, const int n) -> bool {
    return (int(data[n / 8]) & (1 << (n % 8))) != 0;
}

auto get_uinput_device_name(const char* const path) -> Result<std::string> {
    const auto fd = open(path, O_RDONLY);
    if(fd == -1) {
        return Error(build_string("open(", path, ") failed: ", errno));
    }

    auto       buf = std::array<char, 256>();
    const auto len = ioctl(fd, EVIOCGNAME(256), buf.data());
    if(len <= 0) {
        return Error(build_string("ioctl() failed: ", errno));
    }

    return std::string(buf.begin(), buf.begin() + len - 1);
}
} // namespace

auto enumerate_devices() -> std::vector<InputDeviceInfo> {
    auto result = std::vector<InputDeviceInfo>();
    for(auto const& dir_entry : std::filesystem::directory_iterator("/dev/input")) {
        const auto basename = dir_entry.path().filename().string();
        if(!basename.starts_with("event")) {
            continue;
        }

        auto path      = build_string("/dev/input/", basename);
        auto devname_r = get_uinput_device_name(path.data());
        if(!devname_r) {
            warn(devname_r.as_error().cstr());
            continue;
        }
        auto& devname = devname_r.as_value();

        result.emplace_back(InputDeviceInfo{std::move(path), std::move(devname)});
    }

    return result;
}

auto open_uinput_device(const char* const path, bool grab) -> Result<int> {
    const auto fd = open(path, O_RDONLY);
    if(fd == -1) {
        return Error(build_string("open(", path, ") failed: ", errno));
    }
    if(grab) {
        if(ioctl(fd, EVIOCGRAB, 1) != 0) {
            return Error(build_string("ioctl() failed: ", errno));
        }
    }
    return fd;
}

auto create_virtual_device(const std::span<int> parent_device_fds, const char* const device_name) -> Result<int> {
    const auto fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd == -1) {
        return Error(build_string("open(/dev/uinput) failed: ", errno));
    }

    for(const auto pfd : parent_device_fds) {
        const auto events = get_uinput_device_event_bits<0, EV_MAX>(pfd).unwrap();

        auto inherit_all_events = [fd, pfd, &events]<int event, size_t max, int setbit>() -> void {
            if(!check_bit(events, event)) {
                return;
            }

            dynamic_assert(ioctl(fd, UI_SET_EVBIT, event) == 0, "ioctl() failed");

            if constexpr(max != 0) {
                const auto keys = get_uinput_device_event_bits<event, max>(pfd).unwrap();
                for(auto i = size_t(0); i < max; i += 1) {
                    if(check_bit(keys, i)) {
                        dynamic_assert(ioctl(fd, setbit, i) == 0, "ioctl() failed");
                    }
                }
            }
        };

        inherit_all_events.template operator()<EV_SYN, 0, 0>();
        inherit_all_events.template operator()<EV_KEY, KEY_MAX, UI_SET_KEYBIT>();
        inherit_all_events.template operator()<EV_REL, REL_MAX, UI_SET_RELBIT>();
        inherit_all_events.template operator()<EV_MSC, MSC_MAX, UI_SET_MSCBIT>();
        inherit_all_events.template operator()<EV_SW, SW_MAX, UI_SET_SWBIT>();
        inherit_all_events.template operator()<EV_LED, LED_MAX, UI_SET_LEDBIT>();
        inherit_all_events.template operator()<EV_SND, SND_MAX, UI_SET_SNDBIT>();
        inherit_all_events.template operator()<EV_REP, 0, 0>();
        inherit_all_events.template operator()<EV_FF, FF_MAX, UI_SET_FFBIT>();
        inherit_all_events.template operator()<EV_PWR, 0, 0>();
        inherit_all_events.template operator()<EV_FF_STATUS, 0, 0>();
    }

    auto setup = uinput_setup{
        .id = {
            .bustype = BUS_USB,
            .vendor  = 0,
            .product = 0,
            .version = 1,
        },
        .ff_effects_max = 0,
    };
    strncpy(setup.name, device_name, UINPUT_MAX_NAME_SIZE);

    dynamic_assert(ioctl(fd, UI_DEV_SETUP, &setup) == 0, "ioctl() failed");
    dynamic_assert(ioctl(fd, UI_DEV_CREATE) == 0, "ioctl() failed");
    return fd;
}
