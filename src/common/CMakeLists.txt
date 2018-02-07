include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/otsdaq-cmsoutertracker/DTCSupervisor/include)
include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/xdaq/daq/interface/evb/include)
include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/xdaq/daq/interface/shared/include)
include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/xdaq/daq/xdaq2rc/include)
include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/uhal/uhal_2_4_2/cactuscore/uhal/uhal/include)
include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/uhal/uhal_2_4_2/cactuscore/uhal/log/include)
include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/uhal/uhal_2_4_2/cactuscore/uhal/grammars/include)
include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/otsdaq-cmsoutertracker/Ph2_ACF)
include_directories($ENV{OTSDAQ_CMSOUTERTRACKER_DIR}/uhal/uhal_2_4_2/cactuscore/extern/pugixml/RPMBUILD/SOURCES/include/)
include_directories(/usr/include/libxml++-2.6)
include_directories(/usr/lib64/libxml++-2.6/include)
include_directories(/usr/include/glibmm-2.4)
include_directories(/usr/lib64/glibmm-2.4/include)
include_directories(/usr/include/glib-2.0)
include_directories(/usr/lib64/glib-2.0/include)

set_source_files_properties(I2OMsg.cc SLinkI2OSender.cc PROPERTIES HEADER_FILE_ONLY TRUE)

#need to set the OTSDAQ compiler flag in order to build the OTSDAQ specific code
cet_set_compiler_flags(
    EXTRA_FLAGS -Wno-reorder -Wno-overloaded-virtual -Wl,--undefined -D__OTSDAQ__
)

cet_make(LIBRARY_NAME DTCSupervisor
		 LIBRARIES
		 xml++-2.6
		 xslt
		 xdaq2rc
		 Ph2_System
         Ph2_Interface
         Ph2_Tools
         Ph2_Utils
         Ph2_Description
         cactus_uhal_uhal
         cactus_uhal_log
         cactus_uhal_grammars 
		 ${Boost_SYSTEM_LIBRARY}
	     ${ROOT_BASIC_LIB_LIST}
)

install_headers()
install_source()
