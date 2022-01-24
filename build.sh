#!/bin/bash

chmod +x ./build.sh

set -xe

CFLAGS="-Wall -Werror -ggdb `pkg-config --cflags sdl2`"

LIBS="`pkg-config --libs sdl2`"

cc $CFLAGS -o whine main.c $LIBS