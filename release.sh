#!/bin/bash

if test -f "Makefile"; then
	make distclean;
fi

echo Build WinXP release
./configure --with-winxp
make clean
make installer-win

echo Build Win7 release
./configure --with-winxp=no
make clean
make installer-win

