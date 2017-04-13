
What is it
==========

Ezthumb is used to create thumbnails for video files. It uses the FFMPEG 
library as its engine so it almost supports any video formats. 
Normally the screenshots would be gridded into a single canvas picture or 
compose a single GIF animation file.  But it can also save frames separately.
The output format could be JPG, PNG, GIF or GIF animation, where PNG and GIF 
supports the transparent background. Supported batch processing with logarithm
curve profiles.


Release Notes
=============
Version 3.6.0:
*) Building system moved to autoconf/automake.
*) Added a progress bar dialog for integrating with file managers.
*) Supported libav.
*) Fixed the uneven vertical size of buttons in GTK3.

Version 3.4.1:
*) Added factory reset
*) IUP updated to 3.10.1
*) Fixed the progress bar overlap status bar issue
*) Fixed the utf-8 filename issue in Windows
*) Fixed the core dump when verbose level reach 7
*) Fixed the failure of removing registry in Windows.

Version 3.4.0:
*) Added the about page
*) Added the font choice dialog.
*) Fixed the adjustment of the window size.
*) Fixed many things in UI.

Version 3.3.0:
*) FFMPEG updated to 2015-03-03 git-5de2dab (H.265 ready)
*) Accurate PTS display
*) Default duration mode changed to HEAD for ArgusTV.
*) FFMPEG version auto-detection make it easier to build.
*) UI migrated to IUP.
*) Fixed: seperated frame buffer to grab clear image.
*) Fixed: configuration stored in registry in Win32.
*) Fixed: the AR correction issue.
*) Fixed: size incorrect in 32-bit OS when video size over 2GB

Version 3.2.0:
*) ezthumb migrated to a portable GUI based on IUP. 
   The command line interface is still the same.

Version 3.0.4:
*) Fixed a bug in Win32 version that ezthumb went to full scan mode while a
   media player is playing the same file.

Version 3.0.3:
*) Updated the previous fix to prevent ezthumb drop into scan mode unreasonably.

Version 3.0.2:
*) Fixed a bug in the improved-quick-scan which sought out of the video range.

Version 3.0.1:
*) Improved method of quick scan to find the duration of video files
*) Fixed a display bug that file size was showed wrong.

Version 3.0.0:
*) Binding mode can generate one large thumbnail from multiple video sources.
*) Support environment variable 'EZTHUMB' as part of command line arguments.
*) Improved skim mode fits most of video files.
*) Included a test script to help automatic test.
*) Included an internal file name filter to help the recursive mode.
*) Many bug fixes.

Version 2.0.0:
*) ezthumb introduced a GUI based on GTK2. 
   The command line interface is still the same.


Install
=======

Ezthumb requires the following packages:

ffmpeg
gd
gtk2 (in X-window)
freetype
libpng
libjpeg
bzip2

Most systems have already installed those libraries so normally you only need
to install the development files of ffmpeg/gd/gtk2.

*) CentOS 5/6/7 or similar version Fedora/RHEL

$ sudo yum install ffmpeg-devel gd-devel gtk2-devel

then

$ make

*) CentOS 4 or similar version Fedora/RHEL

Current version IUP is not compatible with CentOS 4 so Ezthumb has to disable
the GUI interface. What's more, ezthumb could not generate GIF animation 
thumbnails because the old version of libgd.

$ sudo yum install ffmpeg-devel gd-devel
$ export SYSGUI=CFG_GUI_OFF

then

$ make


*) Windows and MinGW

The ezthumb has been shipped with a library bundle in which all necessary 
libraries are included. You don't need to download anything to build ezthumb.
Just go to the ezthumb directory in MinGW console and type:

make

it will build two execute files, ezthumb.exe for command line and the GUI
frontend ezthumb_win.exe. You may need to use

make install_win

to collect all required DLL files into one directory for easy releasing.
If you have Nullsoft Installer (NSIS) installed, you may use

make release

to generate an install file.




Credits
=======

FFmpeg Win32 shared build by Kyle Schwarz from Zeranoe's:
http://ffmpeg.zeranoe.com/builds/
Of cause you can find source codes of FFMPEG at
https://www.ffmpeg.org/

Following Libraries were grabbed from GnuWin:
http://sourceforge.net/projects/gnuwin32/files/

gd-2.0.33-1
jpeg-6b-4
libiconv-1.9.2-1
libpng-1.2.37
zlib-1.2.3
freetype-2.3.5-1

The icon SMirC-thumbsup.svg is a public domain under GNU Free Documentation 
License. 
http://commons.wikimedia.org/wiki/File:SMirC-thumbsup.svg

The GUI frontend is based on IUP, a multi-platform toolkit for building 
graphical user interfaces.
http://webserver2.tecgraf.puc-rio.br/iup/

This program was inspired by movie thumbnailer (mtn):
http://sourceforge.net/projects/moviethumbnail/



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


