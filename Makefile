BUILD_HOME:=$(shell pwd)/..
include $(XDAQ_ROOT)/config/mfAutoconf.rules
include $(XDAQ_ROOT)/config/mfDefs.$(XDAQ_OS)

#Project=Ph2_TkDAQ
Package=DTCSupervisor
Sources= DTCSupervisor.cc version.cc

IncludeDirs = \
     $(XDAQ_ROOT)/include \
     $(XDAQ_ROOT)/include/linux \
	 /usr/include/libxslt \
	 /usr/include/libxml2

LibraryDirs = \
     $(XDAQ_ROOT)/x86/lib \
     $(XDAQ_ROOT)/lib \
	 /usr/lib64

ExeternalObjects = -lxml -lxslt

UserCCFlags = -g -fPIC -std=c++11 -Wcpp -Wno-unused-local-typedefs -O0 `xslt-config --cflags --libs`
UserDynamicLinkFlags = -lxslt -lxml2 `xslt-config --cflags --libs`

DynamicLibrary=DTCSupervisor

include $(XDAQ_ROOT)/config/Makefile.rules
include $(XDAQ_ROOT)/config/mfRPM.rules
