#!/bin/sh
MAJOR=`grep LIBCSOUP_VER_MAJOR libcsoup.h | cut -f 2`
MINOR=`grep LIBCSOUP_VER_MINOR libcsoup.h | cut -f 2`
BUGS=`grep LIBCSOUP_VER_BUGFIX libcsoup.h | cut -f 2`
echo $MAJOR.$MINOR.$BUGS
