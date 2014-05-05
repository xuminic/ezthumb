#!/bin/sh
grep LIBCSOUP_VERSION libcsoup.h | cut -d\" -f 2
