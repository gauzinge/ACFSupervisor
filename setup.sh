# CACTUS
export CACTUSLIB=/opt/cactus/lib
export CACTUSINCLUDE=/opt/cactus/include

# BOOST
export BOOST_LIB=/opt/cactus/lib
export BOOST_INCLUDE=/opt/cactus/include/boost

#ROOT
#source /usr/local/bin/thisroot.sh
export ROOTLIB=/usr/lib64/root
#export ROOTSYS=/usr/local/lib/root

#ZMQ
export ZMQ_HEADER_PATH=/usr/include/zmq.hpp

#export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
export XDAQ_ROOT=/opt/xdaq
export XDAQ_DOCUMENT_ROOT=/opt/xdaq/htdocs
export ACFSUPERVISOR_ROOT=$(pwd)
#export PH2ACF_ROOT=/home/xtaldaq/Ph2_ACF
export PH2ACF_ROOT=~/Ph2_ACF
export LD_LIBRARY_PATH=/opt/xdaq/lib:$CACTUSLIB:$BOOST_LIB:$PH2ACF_ROOT/lib:$ROOTLIB:$LD_LIBRARY_PATH

#for rcms
alias test="/opt/xdaq/bin/xdaq.exe -p 41801 -c ~/ACFSupervisor/xml/ACFSupervisor.xml"
#alias test="/opt/xdaq/bin/xdaq.exe -p 50000 -c ~/ACFSupervisor/xml/ACFSupervisor.xml"
