# CACTUS
export CACTUSLIB=/opt/cactus/lib
export CACTUSINCLUDE=/opt/cactus/include

# BOOST
export BOOST_LIB=/opt/cactus/lib
export BOOST_INCLUDE=/opt/cactus/include/boost

#ROOT
#source /usr/local/bin/thisroot.sh
export ROOTLIB=/usr/local/lib/root
#export ROOTSYS=/usr/local/lib/root

#ZMQ
export ZMQ_HEADER_PATH=/usr/include/zmq.hpp

export LD_LIBRARY_PATH=/opt/xdaq/lib:$CACTUSLIB:$BOOST_LIB:$PH2ACF_ROOT/lib:$ROOTLIB:$LD_LIBRARY_PATH
#export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
export XDAQ_ROOT=/opt/xdaq
export XDAQ_DOCUMENT_ROOT=/opt/xdaq/htdocs
export DTCSUPERVISOR_ROOT=$(pwd)
export PH2ACF_ROOT=/afs/cern.ch/user/g/gauzinge/Ph2_ACF

alias test="/opt/xdaq/bin/xdaq.exe -p 40400 -c ~/DTCSupervisor/xml/DTCSupervisor.xml"
