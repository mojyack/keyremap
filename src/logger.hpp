#pragma once
#include "keycodes.hpp"
#include "uinput.hpp"
#include "unistd.h"

inline auto logger(const char* const path) -> int {
    const auto fd = unwrap(open_uinput_device(path, false));

    auto event = input_event();
loop:
    if(read(fd, &event, sizeof(event)) != sizeof(event)) {
        warn("read() failed: ", errno);
        return 1;
    }

    printf("[%ld] ", event.time.tv_sec);
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
