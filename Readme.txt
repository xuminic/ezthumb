
What is it
==========

Ezthumb is used to create thumbnails for video files. It uses the FFMPEG 
library as its engine so it almost supports any video formats. 
Normally the screenshots would be gridded into a single canvas picture or 
compose a single GIF animation file.  But it can also save frames separately.
The output format could be JPG, PNG, GIF or GIF animation, where PNG and GIF 
supports the transparent background. Supported batch processing with logarithm
curve profiles.

From 2.0.0 ezthumb introduced a GUI based on GTK2. The command line interface 
is still the same.

This program was inspired by movie thumbnailer (mtn):
http://sourceforge.net/projects/moviethumbnail/


Install
=======

Ezthumb requires following packages:

ffmpeg
gd
freetype
bzip2

You may need to modify the Makefile, especially the INCDIR and LIBDIR macros,
to point to proper directories.



MinGW
=====

Make.conf setting:

SYSTOOL = mingw
SYSAPI  = CFG_WIN32_API

FFmpeg Win32 shared build by Kyle Schwarz from Zeranoe's:
http://ffmpeg.zeranoe.com/builds/

Following Libraries were grabbed from GnuWin:
http://sourceforge.net/projects/gnuwin32/files/

gd-2.0.33-1
jpeg-6b-4
libiconv-1.9.2-1
libpng-1.2.37
zlib-1.2.3
freetype-2.3.5-1

The GTK+ library was tailored from the All-in-one bundles in 
http://www.gtk.org/download/win32.php

NOTE that these libraries are not part of Ezthumb. They were grabbed from
different websites to here to make life easier. Therefore you don't need to 
download them one by one, just download the recent one big bundle tar ball

ezthumb-libmingw-xxxxxxxx.tar.gz 

along with the ezthumb source codes, untar it to libmingw/
The file structure should look like this:

./ezthumb.c
./libmingw/
./libmingw/gtk-2.24.8/
...

Then you may build it by

make clean
make

It will generate two executable files: ezthumb.exe and ezthumb_win.exe. 
One for the console and another one for the GUI environment. 
These two files can not be executed directly in normal condition because 
the supporting DLLs were not with them. You need to

make install

to collect all files into the directory looks like

ezthumb-x.x.x-win-bin

then

cd ezthumb-x.x.x-win-bin
ezthumb
ezthumb_win

Enjoy!


Icons
=====

The icon file SMirC-thumbsup.svg is a public domain under GNU Free Documentation License.
http://commons.wikimedia.org/wiki/File:SMirC-thumbsup.svg

Examples
========

You may use the command as simple as this to test it: 

ezthumb *.avi

It will generate thumbnails for all .avi files by the default profile.
Another common usage is including a grid setting and a size setting:

ezthumb -g 4x8 -s 33% *.avi

It will generate the 4x8 thumbnails for all .avi files in the current 
directory. Each screen shots inside the thumbnails are 33% of the video frame
in width and height.

This command can list the useful information of video files:

ezthumb -i *.avi

Please see the manpage for more examples.


