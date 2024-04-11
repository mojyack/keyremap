#include <linux/uinput.h>
#include <unistd.h>

#include "keycodes.hpp"
#include "uinput.hpp"

auto view_inputs(const char* const path) -> int {
    const auto fd = open_uinput_device(path, false).unwrap();

    auto event = input_event();
loop:
    if(read(fd, &event, sizeof(event)) != sizeof(event)) {
        warn("read() failed: ", errno);
        return 1;
    }

    printf("[%ld.%ld] %d %d %d\n", event.time.tv_sec, event.time.tv_usec, event.type, event.code, event.value);

    switch(event.type) {
    case EV_SYN:
        printf("SYN\n");
        break;
    case EV_KEY:
        printf("KEY %s ", code2str(event.code));
        switch(event.value) {
        case 0:
            printf("up\n");
            break;
        case 1:
            printf("down\n");
            break;
        case 2:
            printf("repeat\n");
            break;
        }
        break;
    case EV_REL:
        printf("REL\n");
        break;
    case EV_ABS:
        printf("ABS\n");
        break;
    case EV_MSC:
        printf("MSC\n");
        break;
    case EV_LED:
        printf("LED\n");
        break;
    case EV_SND:
        printf("SND\n");
        break;
    case EV_REP:
        printf("REP\n");
        break;
    case EV_FF:
        printf("FF\n");
        break;
    case EV_PWR:
        printf("PWR\n");
        break;
    }

    goto loop;

    return 0;
}

constexpr auto usage = R"(usage:
    input-viewer {/dev/input/event*}    :show inputs of the device
    input-viewer help                   : print this message
)";

auto main(const int argc, const char* argv[]) -> int {
    if(argc != 2) {
        print(usage);
        return 1;
    }
    if(std::string_view(argv[1]) == "help") {
        print(usage);
        return 1;
    }
    return view_inputs(argv[1]);
}
