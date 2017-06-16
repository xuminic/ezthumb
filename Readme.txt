
What is it
==========

Ezthumb is used to create thumbnails for video files. It uses the FFMPEG 
library as its engine so it almost supports any video formats. 

Features
========
*) Supports almost any video formats:
   3gp,3g2,asf,asx,avi,avs,divx,flv,f4v,f4p,f4a,f4b,m1v,m2v,m4p,m4v,mjpg,mkv,
   mov,movie,mp2,mp4,mpa,mpe,mpeg,mpg,mpv,mv,ogv,ogg,qt,rm,rmvb,rv,ts,viv,
   vivo,vob,webm,wmv,yuv
*) Ouput JPG, PNG or GIF format.
*) Support transparent PNG or GIF.
*) Support Animated GIF.
*) Support all thumbnails in one canvas, or individual thumbnails.
*) Support batch processing in command line.
*) Support both command line tool and a lightweight GUI application.
*) Cross platform design support Linux/Windows XP/Windows 7 (Win32 API).
*) Using IUP portable UI library to gain the native GUI feeling.
*) Written in C and FFMPEG, as well as libav, API.

Release Notes
=============
Version 3.6.7:
*) Separated the Win32 build to Windows XP, with 'rogerdpack' FFMPEG build 3.1.2, 
   and Windows 7, with 'Zeranoe' FFMPEG build 3.3.1 and higher.
*) Added more video extension names to the video filter.
*) Added the --size-unit option to customize the preferred units.
*) Fixed the bug which failed to drag & drop files with spaces inside the names (GTK).
*) Fixed the shrinking window issue.

Version 3.6.6:
*) Support new API in FFMPEG 3.x
*) Fixed a bug which failed to install EzthumbWin.exe.

Version 3.6.5:
*) The progress bar dialog will inherit the configuration from the ezthumb GUI dialog.
*) Fixed a bug that Chinese characters were failed to pass to the progress bar dialog.
*) Workarounded the imcomplete context menu in Windows by associates ezthumb to the OpenWithList.

Version 3.6.4:
*) Fixed the bug which freeze the progress bar in i-frame dumping mode.
*) Fixed the bug in configure file to the correct linking sequence.
*) Fixed the bug which failed to hightlight the "RUN" button when media files 
   were loaded from the command line.
*) Improved the user input by verifying the content in the text controls.
*) Added the support of TrueType fonts in Windows.
   Note that the libgd uses fontconfig to translate the font faces to the font
   files. In Windows however there's no fontconfig available so libgd could not
   find the proper font files. Unfortunately there's also no out-of-box APIs to
   directly translate the font faces to the font files in Windows. My approach 
   is to enumerate all registered font faces and to compare them with all 
   registered true type fonts. It's still imperfect but around 95% fonts would 
   be matched.

Version 3.6.3 quick release:
*) Fixed the bug which failed to save the transparent screenshots in GUI.
*) Fixed the linking sequence issue between libiup and libX11.
*) Added the missing 64x64 icon.

Version 3.6.2:
The 3.6.1 build was so broken it must be replaced asap.
*) The 'deprecated build' supports ffmpeg 3.3.
*) Fixed the bug that failed to restore the default value of the Grid Setting as well as the text control detection.
*) Fixed the bug that the label control has a different background colour than the dialog background.
*) Fixed the bug that the zoom option layout is inconsistent in different distros.
*) Fixed the bug which deleting all entries in GTK3.
*) Fixed the bug which generating screenshot with wrong attribution if some videoes were removed from the list control.
*) Fixed the bug which failed to recover the 'auto' zooming if other zooms were chosen once.
*) Fixed the bug which failed to save to png and gif.
*) Fixed the bug which failed to save the suffix to the config file.
*) Fixed the bug which failed to save to the specified path.

Version 3.6.1:
*) Building system moved to autoconf/automake.
*) Added a progress bar dialog for integrating with file managers.
*) Added to the context menu in Windows File Manager.
*) Supported libav.
*) IUP updated to 3.21 and using the official WIN32 libs distributed by IUP.
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

Ezthumb requires the following packages in *nix:

ffmpeg
gd
freetype
libpng
libjpeg
bzip2
gtk (in X-window)

Most systems have already installed those libraries so normally you only need
to install the development files of ffmpeg/gd/gtk.

*) CentOS 5/6/7 or similar version Fedora/RHEL

$ sudo yum install ffmpeg-devel gd-devel gtk2-devel

then

$ ./configure
$ make

*) CentOS 4 or similar version Fedora/RHEL

Current version IUP is not compatible with CentOS 4 so Ezthumb has to disable
the GUI interface. What's more, ezthumb could not generate GIF animation 
thumbnails because the old version of libgd.

$ sudo yum install ffmpeg-devel gd-devel

$ ./configure --with-gui=no
$ make


*) Windows and MinGW

The ezthumb has been shipped with a library bundle in which all necessary 
libraries are included. You don't need to download anything to build ezthumb.
In Windows 7 and higher, just go to the ezthumb directory in MinGW console and type:

$ ./configure
$ make

In Windows XP/Vista, however:

$ ./configure --with-winxp
$ make

it will build two execute files, ezthumb.exe for command line and the GUI
frontend EzthumbWin.exe. You will find the execute files and required DLLs
under the folder "ezthumb-X.X.X-win7-bin" or "ezthumb-X.X.X-winxp-bin"

Note that the FFMPEG has stopped supporting Windows XP. Therefore ezthumb
have to release two packages: the Windows XP release will probably stick on
the 'rogerdpack' build 3.1.2; the Windows 7 release will keep sync-ing with
the recent FFMPEGs.

If you have Nullsoft Installer (NSIS) installed, you may use

./release.sh

to generate two installer files each for Windows XP and Windows 7.




Credits
=======

FFmpeg Win32 shared build by Kyle Schwarz from Zeranoe's:
http://ffmpeg.zeranoe.com/builds/
You may find source codes of FFMPEG at
https://www.ffmpeg.org/

Following Libraries were grabbed from GnuWin:
http://sourceforge.net/projects/gnuwin32/files/

freetype-2.6.3
gd-2.0.35
jpeg-6b
libpng-1.5.13
zlib-1.2.8
libiconv-1.9.2.1

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


