ffmpeg_3.1.2_32bit_shared_for WinXP

This is the 'rogerdpack' build extracted from:
https://sourceforge.net/projects/ffmpegwindowsbi/files/2016-08-12-v3.1.2/

new in this release: 
shared libraries (for the win!) 
built 2016-08-13-v3.1.2-g4275b27 
using ffmpeg-windows-build-helpers v56af56e 
see the github distro at that commit if you want to know which dependencies and 
their versions were used 
also see https://github.com/rdp/ffmpeg-windows-build-helpers if you need/want instructions 
to build your own modified version

The avfilter-6.dll and its library were replaced by the same file in 3.3.1 Win32 build
because the 'rogerdpack' build were so hugh, 23mb for this dll though ezthumb doesn't use it at all.