BUILD_HOME:=$(shell pwd)/..
include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

ROOTHTTP := $(shell root-config --has-http)
ROOTVERSION := $(shell root-config --version)

##################################################
## check if Root has Http
##################################################
ifneq (,$(findstring yes,$(ROOTHTTP)))
	RootExtraLinkFlags= -lRHTTP
	RootExtraFlags=-D__HTTP__
else
	RootExtraLinkFlags=
	RootExtraFlags=
endif
##################################################
## check Root version
##################################################
ifneq (,$(findstring 6.1,$(ROOTVERSION)))
	RootExtraFlags+=-D__ROOT6__
endif

#Project=Ph2_TkDAQ
Package=DTCSupervisor
Sources= DTCSupervisor.cc SupervisorGUI.cc DTCStateMachine.cc TCPDataSender.cc version.cc XMLUtils.cc #LogReader.h

IncludeDirs = \
	 ${BOOST_INCLUDE} \
     $(XDAQ_ROOT)/include \
     $(XDAQ_ROOT)/include/linux \
	 ${PH2ACF_ROOT} \
	 ${CACTUSINCLUDE} \
	 /usr/include

LibraryDirs = \
     $(XDAQ_ROOT)/x86/lib \
     $(XDAQ_ROOT)/lib \
	 ${PH2ACF_ROOT}/lib \
	 ${CACTUSLIB} \
	 ${ROOTLIB} \
	 /usr/lib64

ExeternalObjects =

#UserCCFlags = -g -fPIC -std=c++11 -Wcpp -Wno-unused-local-typedefs -Wno-reorder -O0 $(shell pkg-config --cflags libxml++-2.6) $(shell xslt-config --cflags) $(shell root-config --cflags)
UserCCFlags = -g -O1 -fPIC -std=c++11 -w -Wall -pedantic -pthread -Wcpp -Wno-reorder -O0 $(shell pkg-config --cflags libxml++-2.6) $(shell xslt-config --cflags) $(shell root-config --cflags) $(RootExtraFlags)
UserDynamicLinkFlags = ${LibraryPaths} $(shell pkg-config --libs libxml++-2.6) $(shell xslt-config --libs)  $(shell root-config --libs) -lxdaq2rc -lPh2_Utils -lPh2_Description -lPh2_Interface -lPh2_System -lPh2_Tools $(RootExtraLinkFlags)

#$(LibraryPaths)

DynamicLibrary=DTCSupervisor

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules
