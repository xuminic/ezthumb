#!/bin/sh
emcc -I ../../../include main_c.c -s NO_EXIT_RUNTIME=1 -L ../../../lib/Linux48_64 -liup -o hello.html


