BUILD_HOME:=$(shell pwd)/..
include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

#Project=Ph2_TkDAQ
Package=DTCSupervisor
Sources= DTCSupervisor.cc SupervisorGUI.cc DTCStateMachine.cc version.cc XMLUtils.cc

IncludeDirs = \
     $(XDAQ_ROOT)/include \
     $(XDAQ_ROOT)/include/linux \
	 /usr/include

LibraryDirs = \
     $(XDAQ_ROOT)/x86/lib \
     $(XDAQ_ROOT)/lib \
	 /usr/lib64

ExeternalObjects =

UserCCFlags = -g -fPIC -std=c++11 -Wcpp -Wno-unused-local-typedefs -O0 $(shell pkg-config --cflags libxml++-2.6) $(shell xslt-config --cflags) #$(shell xml2-config --cflags)
UserDynamicLinkFlags = $(shell pkg-config --libs libxml++-2.6) $(shell xslt-config --libs) # $(shell xml2-config --libs) -lxalan-c 


DynamicLibrary=DTCSupervisor

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules
