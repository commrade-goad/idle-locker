#!/usr/bin/env sh
[ -d "build" ] || mkdir -p build

gcc \
    main.c \
    helper.c \
    idler.c \
    -o build/idle-locker \
    $(pkg-config --cflags --libs x11 xext xscrnsaver dbus-1) \
    -lpthread -Wall -Wextra -pedantic -O2
