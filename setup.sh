export XDAQ_ROOT=/opt/xdaq
export LD_LIBRARY_PATH=/opt/xdaq/lib
export XDAQ_DOCUMENT_ROOT=/opt/xdaq/htdocs
export DTCSUPERVISOR_ROOT=$(pwd)
alias test="/opt/xdaq/bin/xdaq.exe -p 40400 -c ~/DTCSupervisor/xml/DTCSupervisor.xml"
