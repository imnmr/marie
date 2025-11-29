#!/usr/bin/env sh

set -e

CC="${CC:-cc}"
CFLAGS="-std=c11 -g -Wall -Wextra -Wpedantic"

"$CC" $CFLAGS -o marie main.c
