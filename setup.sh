export LD_LIBRARY_PATH=/usr/local/lib:/opt/xdaq/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
export XDAQ_ROOT=/opt/xdaq
export XDAQ_DOCUMENT_ROOT=/opt/xdaq/htdocs
export DTCSUPERVISOR_ROOT=$(pwd)
export PH2ACF_ROOT=/afs/cern.ch/user/g/gauzinge/Ph2_ACF
echo ${PH2ACF_ROOT}
alias test="/opt/xdaq/bin/xdaq.exe -p 40400 -c ~/DTCSupervisor/xml/DTCSupervisor.xml"
