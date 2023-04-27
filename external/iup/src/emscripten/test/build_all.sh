#!/bin/sh
mkdir -p BUILD
emcc -O0 -g -I ../../../include -I ../.. -I ..  ../../*.c ../*.c main_c.c --js-library ../iupemscripten_common.js --js-library ../iupemscripten_button.js --js-library ../iupemscripten_label.js --js-library ../iupemscripten_dialog.js -s NO_EXIT_RUNTIME=1  -o BUILD/hello.html

