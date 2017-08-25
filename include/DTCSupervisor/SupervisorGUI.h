#ifndef __SupervisorGUI_H__
#define __SupervisorGUI_H__

#include "xdaq/exception/Exception.h"
#include "xdaq/NamespaceURI.h"
#include "toolbox/lang/Class.h"
#include "toolbox/lang/Method.h"

#include "xgi/Utils.h"
#include "xgi/Method.h"
#include "xgi/framework/UIManager.h"

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

    class SupervisorGUI : public toolbox::lang::Class
    {
      public:
        SupervisorGUI (xgi::framework::UIManager* pManager, std::string& pURN);

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


      public:
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


        //members
      protected:
        xgi::framework::UIManager* fManager;
        std::string fURN;
      public:
        xdata::String* fHWDescriptionFile;
        xdata::String* fXLSStylesheet;

        std::string fHWFormString;

        Tab fCurrentPageView;
    };
}
#endif
