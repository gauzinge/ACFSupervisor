#ifndef __DTCSupervisor_H__
#define __DTCSupervisor_H__

#include "xdaq/Application.h"
#include "xgi/Method.h"
#include "cgicc/HTMLClasses.h"

namespace Ph2TkDAQ {

    class DTCSupervisor : public xdaq::Application
    {
      public:
        XDAQ_INSTANTIATOR();

        DTCSupervisor (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
        void Default (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
    };

}

#endif
