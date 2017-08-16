#include "DTCSupervisor/DTCSupervisor.h"


XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)

Ph2TkDAQ::DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::Application (s)
{
    xgi::bind (this, &DTCSupervisor::Default, "Default");
}

void Ph2TkDAQ::DTCSupervisor::Default (xgi::Input* in, xgi::Output* out)
throw (xgi::exception::Exception)
{
    LOG4CPLUS_INFO (this->getApplicationLogger(), "Default method called!");
    *out << cgicc::HTMLDoctype (cgicc::HTMLDoctype::eStrict) << std::endl;
    *out << cgicc::html().set ("lang", "en").set ("dir", "ltr") << std::endl;
    *out << cgicc::title ("DTC Supervisor") << std::endl;
    *out << cgicc::a ("Visit the XDAQ Web site").set ("href", "http://xdaq.web.cern.ch") << std::endl;
}
