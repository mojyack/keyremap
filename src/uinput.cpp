#include <cstring>
#include <filesystem>

#include <fcntl.h>
#include <linux/uinput.h>

#include "macros/unwrap.hpp"
#include "uinput.hpp"
#include "util/fd.hpp"

namespace {
template <size_t bits>
constexpr auto bits_to_bytes = (bits - 1) / 8 + 1;

template <int event, size_t max>
auto get_uinput_device_event_bits(const int fd) -> std::optional<std::array<std::byte, bits_to_bytes<max>>> {
    auto result = std::array<std::byte, bits_to_bytes<max>>();
    ensure(ioctl(fd, EVIOCGBIT(event, result.size()), result.data()) == result.size(), "ioctl() failed: ", strerror(errno));
    return result;
}

template <size_t size>
auto check_bit(const std::array<std::byte, size>& data, const int n) -> bool {
    return (int(data[n / 8]) & (1 << (n % 8))) != 0;
}

auto get_uinput_device_name(const char* const path) -> std::optional<std::string> {
    const auto fd = FileDescriptor(open(path, O_RDONLY));
    ensure(fd.as_handle() >= 0, "failed to open ", path, ": ", strerror(errno));
    auto       buf = std::array<char, 256>();
    const auto len = ioctl(fd.as_handle(), EVIOCGNAME(256), buf.data());
    ensure(len > 0, "ioctl() failed: ", strerror(errno));
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

        const auto path = build_string("/dev/input/", basename);
        if(auto devname = get_uinput_device_name(path.data())) {
            result.emplace_back(InputDeviceInfo{std::move(path), std::move(*devname)});
        }
    }

    return result;
}

auto open_uinput_device(const char* const path, bool grab) -> std::optional<FileDescriptor> {
    auto fd = FileDescriptor(open(path, O_RDONLY));
    ensure(fd.as_handle() >= 0, "failed to open ", path, ": ", strerror(errno));
    if(grab) {
        ensure(ioctl(fd.as_handle(), EVIOCGRAB, 1) == 0, "ioctl() failed: ", strerror(errno));
    }
    return fd;
}

auto configure_virtual_device(const std::span<int> parent_device_fds) -> std::optional<FileDescriptor> {
    auto fd = FileDescriptor(open("/dev/uinput", O_WRONLY | O_NONBLOCK));
    ensure(fd.as_handle() >= 0, "failed to open /dev/uniput: ", strerror(errno));
    for(const auto pfd : parent_device_fds) {
        auto events_oo = get_uinput_device_event_bits<0, EV_MAX>(pfd);
        unwrap(events, events_oo);

        const auto inherit_all_events = [&fd, pfd, &events]<int event, size_t max = 0, int setbit = 0>() -> bool {
            constexpr auto error_value = false;

            if(!check_bit(events, event)) {
                return true;
            }

            ensure_v(ioctl(fd.as_handle(), UI_SET_EVBIT, event) == 0, "ioctl() failed: ", strerror(errno));

            if constexpr(max != 0) {
                auto keys_oo = get_uinput_device_event_bits<event, max>(pfd);
                unwrap_v(keys, keys_oo);
                for(auto i = size_t(0); i < max; i += 1) {
                    if(check_bit(keys, i)) {
                        ensure_v(ioctl(fd.as_handle(), setbit, i) == 0, "ioctl() failed: ", strerror(errno));
                    }
                }
            }
            return true;
        };

        inherit_all_events.template operator()<EV_SYN>();
        inherit_all_events.template operator()<EV_KEY, KEY_MAX, UI_SET_KEYBIT>();
        inherit_all_events.template operator()<EV_REL, REL_MAX, UI_SET_RELBIT>();
        inherit_all_events.template operator()<EV_MSC, MSC_MAX, UI_SET_MSCBIT>();
        inherit_all_events.template operator()<EV_SW, SW_MAX, UI_SET_SWBIT>();
        inherit_all_events.template operator()<EV_LED, LED_MAX, UI_SET_LEDBIT>();
        inherit_all_events.template operator()<EV_SND, SND_MAX, UI_SET_SNDBIT>();
        inherit_all_events.template operator()<EV_REP>();
        inherit_all_events.template operator()<EV_FF, FF_MAX, UI_SET_FFBIT>();
        inherit_all_events.template operator()<EV_PWR>();
        inherit_all_events.template operator()<EV_FF_STATUS>();
    }
    return fd;
}

auto create_virtual_device(const int fd, const char* const device_name) -> bool {
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

    ensure(ioctl(fd, UI_DEV_SETUP, &setup) == 1, "ioctl() failed: ", strerror(errno));
    ensure(ioctl(fd, UI_DEV_CREATE) == 0, "ioctl() failed: ", strerror(errno));
    return true;
}
