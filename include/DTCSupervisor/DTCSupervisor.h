#ifndef __DTCSupervisor_H__
#define __DTCSupervisor_H__

#include "xdaq/exception/Exception.h"
#include "xdaq/NamespaceURI.h"
#include "xdaq/WebApplication.h"

#include "xgi/Utils.h"
#include "xgi/Method.h"

#include "cgicc/CgiDefs.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"

#include "xdata/ActionListener.h"
#include "xdata/String.h"
#include "xdata/Integer.h"
#include "xdata/Boolean.h"

#include "DTCSupervisor/SupervisorGUI.h"
#include "DTCSupervisor/DTCStateMachine.h"

//Ph2 ACF Includes
#include "System/SystemController.h"
#include "tools/Tool.h"
#include "tools/Calibration.h"
#include "tools/PedeNoise.h"
#include "tools/LatencyScan.h"
#include "Utils/Utilities.h"
#include "Utils/easylogging++.h"

#include <regex>

using FormData = std::map<std::string, std::string>;

namespace Ph2TkDAQ {

    //DTC Supervisor main class, responsible for GUI and user input
    class DTCSupervisor : public xdaq::WebApplication, public xdata::ActionListener//, public xgi::framework::UIManager
    {
      public:
        XDAQ_INSTANTIATOR();
        //the GUI object
        SupervisorGUI* fGUI;
        DTCStateMachine fFSM;

        DTCSupervisor (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
        ~DTCSupervisor();

        //xdata:event listener
        void actionPerformed (xdata::Event& e);

        //views
        void Default (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);

      protected:
        xdata::String fHWDescriptionFile;
        xdata::String fDataDirectory;
        xdata::String fResultDirectory;
        xdata::Integer fRunNumber;
        xdata::Integer fNEvents;
        xdata::Boolean fRAWFile;
        xdata::Boolean fDAQFile;

      private:
        FormData fHWFormData;
        FormData fSettingsFormData;
        Ph2_System::SystemController fSystemController;
        //File Handler for SLINK Data - the one for raw data is part of SystemController
        FileHandler* fSLinkFileHandler;

        //callbacks for FSM states
      public:
        bool initialising (toolbox::task::WorkLoop* wl);
        ///Perform configure transition
        bool configuring (toolbox::task::WorkLoop* wl);
        ///Perform enable transition
        bool enabling (toolbox::task::WorkLoop* wl);
        ///Perform halt transition
        bool halting (toolbox::task::WorkLoop* wl);
        ///perform pause transition
        bool pausing (toolbox::task::WorkLoop* wl);
        ///Perform resume transition
        bool resuming (toolbox::task::WorkLoop* wl);
        ///Perform stop transition
        bool stopping (toolbox::task::WorkLoop* wl);
        ///Perform destroy transition
        bool destroying (toolbox::task::WorkLoop* wl);

        /** FSM call back   */
        xoap::MessageReference fsmCallback (xoap::MessageReference msg) throw (xoap::exception::Exception)
        {
            return fFSM.commandCallback (msg);
        }

      private:
        void updateHwDescription ();
        void updateSettings ();

    };

}

#endif
