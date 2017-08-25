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
#include "XMLUtils.h"
#include "ConsoleColor.h"

enum class Tab {CONFIG, MAIN, CALIBRATION, DAQ};

namespace Ph2TkDAQ {

    //DTC Supervisor main class, responsible for GUI and user input
    class DTCSupervisor : public xdaq::WebApplication, public xdata::ActionListener//, public xgi::framework::UIManager
    {
      public:
        XDAQ_INSTANTIATOR();
        xgi::framework::UIManager fManager;

        DTCSupervisor (xdaq::ApplicationStub* s) throw (xdaq::exception::Exception);
        ~DTCSupervisor();

        //xdata:event listener
        void actionPerformed (xdata::Event& e);

        //views
        void Default (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void MainPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void ConfigPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void CalibrationPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception) {}
        void DAQPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception) {}

        ///HTML header & footer for Hyperdaq interface
        void createHtmlHeader (xgi::Input* in, xgi::Output* out, Tab pTab);
        void createHtmlFooter (xgi::Input* in, xgi::Output* out);
        // Form to load HWDescription File
        void displayLoadForm (xgi::Input* in, xgi::Output* out);

        ///Show FSM status and available transition in an HTML table
        void showStateMachineStatus (xgi::Output* out) throw (xgi::exception::Exception);

      protected:
        xdata::String fHWDescriptionFile;
        xdata::String fXLSStylesheet;
        std::string fHWFormString;

      private:
        void reloadHWFile (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void handleHWFormData (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);



        void lastPage (xgi::Input* in, xgi::Output* out)
        {
            switch (fCurrentPageView)
            {
                case Tab::MAIN:
                    this->MainPage (in, out);
                    break;

                case Tab::CONFIG:
                    this->ConfigPage (in, out);
                    break;

                case Tab::CALIBRATION:
                    this->CalibrationPage (in, out);
                    break;

                case Tab::DAQ:
                    this->DAQPage (in, out);
                    break;
            }
        }


        Tab fCurrentPageView;
        std::vector<std::pair<std::string, std::string>> fHWFormVector;

    };

}

#endif
