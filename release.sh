#!/bin/sh

VERSION=`grep EZTHUMB_VERSION ezthumb.h | cut -d'"' -f 2`
DATE=`date  +%Y%m%d`

pack_source() {
	mkdir ezthumb-$VERSION
	for i in *; do
		if [ -f $i ]; then
			cp $i ezthumb-$VERSION
		fi
	done
	cp -a libsmm ezthumb-$VERSION
	tar czf ezthumb-$VERSION.tar.gz ezthumb-$VERSION
	rm -rf ezthumb-$VERSION
}

pack_win_extra() {
	tar czf ezthumb-libmingw-$DATE.tar.gz libmingw
}

pack_win_binary() {
	if [ -d release-bin ]; then
		mv release-bin ezthumb-$VERSION-win-bin
		tar czf ezthumb-$VERSION-win-bin.tar.gz ezthumb-$VERSION-win-bin
		rm -rf ezthumb-$VERSION-win-bin
	fi
}


pack_source
if [ "$1" = "win" ]; then
	pack_win_extra
	pack_win_binary
fi


