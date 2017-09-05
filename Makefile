BUILD_HOME:=$(shell pwd)/..
include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

#Project=Ph2_TkDAQ
Package=DTCSupervisor
Sources= DTCSupervisor.cc SupervisorGUI.cc DTCStateMachine.cc version.cc XMLUtils.cc #LogReader.h

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
	 /usr/lib64

ExeternalObjects =

UserCCFlags = -g -fPIC -std=c++11 -Wcpp -Wno-unused-local-typedefs -Wno-reorder -O0 $(shell pkg-config --cflags libxml++-2.6) $(shell xslt-config --cflags) $(shell root-config --cflags)
UserDynamicLinkFlags = ${LibraryPaths} $(shell pkg-config --libs libxml++-2.6) $(shell xslt-config --libs)  $(shell root-config --evelibs) -lxdaq2rc -lPh2_Utils -lPh2_Description -lPh2_Interface -lPh2_System -lPh2_Tools 

$(LibraryPaths)

DynamicLibrary=DTCSupervisor

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules
