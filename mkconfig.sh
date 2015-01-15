#!/bin/sh

echo "#ifndef	_EZCONFIG_H_"
echo "#define	_EZCONFIG_H_"
echo ""

rm -f conftest
make conftest CONTEST=TEST_AV_FRAME_ALLOC >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AV_FRAME_ALLOC		1"
else
	echo "#define	HAVE_AVCODEC_ALLOC_FRAME	1"
fi

rm -f conftest
make conftest CONTEST=TEST_AVFORMAT_OPEN_INPUT >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AVFORMAT_OPEN_INPUT	1"
else
	echo "#define	HAVE_AV_OPEN_INPUT_FILE		1"
fi

rm -f conftest
make conftest CONTEST=TEST_AVFORMAT_FIND_STREAM_INFO  >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AVFORMAT_FIND_STREAM_INFO	1"
else
	echo "#define	HAVE_AV_FIND_STREAM_INFO	1"
fi

rm -f conftest
make conftest CONTEST=TEST_AVCODEC_OPEN2 >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AVCODEC_OPEN2		1"
else
	echo "#define	HAVE_AVCODEC_OPEN		1"
fi

rm -f conftest
make conftest CONTEST=TEST_AV_FIND_BEST_STREAM >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AV_FIND_BEST_STREAM	1"
fi

rm -f conftest
make conftest CONTEST=TEST_AV_DICT_GET >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AV_DICT_GET		1"
fi

rm -f conftest
make conftest CONTEST=TEST_AV_METADATA_GET >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AV_METADATA_GET		1"
fi


rm -f conftest
make conftest CONTEST=TEST_AV_DUMP_FORMAT >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AV_DUMP_FORMAT		1"
else
	echo "#define	HAVE_DUMP_FORMAT		1"
fi

rm -f conftest
make conftest CONTEST=TEST_AVFORMATCONTEXT_PB >> conftest.log 2>> conftest.log
if test "$?" = "0"; then
	echo "#define	HAVE_AVFORMATCONTEXT_PB		1"
else
	echo "#define	HAVE_AVFORMATCONTEXT_FILE_SIZE	1"
fi

rm -f conftest
echo ""
echo "#endif"

