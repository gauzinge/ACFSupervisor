#ifndef __DTCSupervisor_H__
#define __DTCSupervisor_H__

#include "xdaq/Application.h"
#include "xdaq/exception/Exception.h"
#include "xdaq/NamespaceURI.h"
#include "xdaq/WebApplication.h"

#include "xgi/Utils.h"
#include "xgi/Method.h"
//#include "xgi/framework/UIManager.h"

#include "cgicc/CgiDefs.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"

#include "xdata/ActionListener.h"
#include "xdata/String.h"

#include "Utils.h"
#include "ConsoleColor.h"

enum class Tab {CONFIG, MAIN, CALIBRATION, DAQ};

namespace Ph2TkDAQ {

    //DTC Supervisor main class, responsible for GUI and user input
    class DTCSupervisor : public xdaq::WebApplication, public xdata::ActionListener//, public xgi::framework::UIManager
    {
      public:
        XDAQ_INSTANTIATOR();

        DTCSupervisor (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
        ~DTCSupervisor();

        //xdata:event listener
        void actionPerformed (xdata::Event& e);

        //views
        void Default (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void MainPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void ConfigPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //void CalibrationPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //void DAQPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);

        ///HTML header for Hyperdaq interface
        void createHtmlHeader (xgi::Output* out, Tab pTab, const std::string& strDest = ".");
        ///Show FSM status and available transition in an HTML table
        void showStateMachineStatus (xgi::Output* out) throw (xgi::exception::Exception);

        void loadHWDescriptionFile (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
      protected:
        xdata::String fHWDescriptionFile;
        xdata::String fXLSStylesheet;
        //xgi::framework::UIManager fManager;

      private:

        Tab fCurrentPageView;

    };

}

#endif
