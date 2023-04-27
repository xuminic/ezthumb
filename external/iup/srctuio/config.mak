PROJNAME = iup
LIBNAME  = iuptuio
OPT = YES

ifdef DBG
  DEFINES += IUP_ASSERT
endif  

INCLUDES = ../include ../src tuio oscpack
LDIR = ../lib/$(TEC_UNAME)  
LIBS = iup

TUIO := \
  FlashSender.cpp TcpReceiver.cpp TuioClient.cpp TuioDispatcher.cpp TuioPoint.cpp UdpReceiver.cpp \
  OneEuroFilter.cpp TcpSender.cpp TuioContainer.cpp TuioManager.cpp TuioServer.cpp UdpSender.cpp \
  OscReceiver.cpp TuioBlob.cpp TuioCursor.cpp TuioObject.cpp TuioTime.cpp WebSockSender.cpp
TUIO := $(addprefix tuio/, $(TUIO))

OSC_IP_WIN32 = oscpack/ip/win32/NetworkingUtils.cpp oscpack/ip/win32/UdpSocket.cpp
OSC_IP_POSIX = oscpack/ip/posix/NetworkingUtils.cpp oscpack/ip/posix/UdpSocket.cpp
OSC_IP = oscpack/ip/IpEndpointName.cpp
OSC = oscpack/osc/OscTypes.cpp oscpack/osc/OscOutboundPacketStream.cpp \
      oscpack/osc/OscReceivedElements.cpp oscpack/osc/OscPrintReceivedElements.cpp 

SRC := $(TUIO) $(OSC) $(OSC_IP) iup_tuio.cpp

ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  SRC += $(OSC_IP_WIN32)
  LIBS += ws2_32 winmm
else
  SRC += $(OSC_IP_POSIX)
endif

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  ifneq ($(TEC_SYSMINOR), 4)
    BUILD_DYLIB=Yes
  endif
endif

ifeq ($(TEC_BYTEORDER), TEC_LITTLEENDIAN)
  DEFINES += OSC_HOST_LITTLE_ENDIAN
else
  DEFINES += OSC_HOST_BIG_ENDIAN
endif

#When building shared libraries in UNIX?
# -lpthread
