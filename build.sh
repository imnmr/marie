#!/usr/bin/env sh

set -e

CC="${CC:-cc}"
CFLAGS="-std=c99 -g -ggdb -Wall -Wextra -Wpedantic"

"$CC" $CFLAGS -o marie main.c
