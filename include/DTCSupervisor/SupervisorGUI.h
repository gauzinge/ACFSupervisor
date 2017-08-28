#ifndef __SupervisorGUI_H__
#define __SupervisorGUI_H__

#include "xdaq/WebApplication.h"
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

#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"

#include "DTCSupervisor/DTCStateMachine.h"

#include "Utils.h"
#include "XMLUtils.h"
#include "ConsoleColor.h"

using FormData = std::map<std::string, std::string>;

enum class Tab {CONFIG, MAIN, CALIBRATION, DAQ};

namespace Ph2TkDAQ {

    class SupervisorGUI : public toolbox::lang::Class
    {
      public:
        SupervisorGUI (xdaq::WebApplication* pApp, DTCStateMachine* pStateMachine);

        //views
        void Default (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void MainPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void ConfigPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void CalibrationPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception) {}
        void DAQPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception) {}

        ///HTML header & footer for Hyperdaq interface
        void createHtmlHeader (xgi::Input* in, xgi::Output* out, Tab pTab);
        void createHtmlFooter (xgi::Input* in, xgi::Output* out);
        ///Show FSM status and available transition in an HTML table
        void showStateMachineStatus (xgi::Output* out) throw (xgi::exception::Exception);

        // Form to load HWDescription File
        void displayLoadForm (xgi::Input* in, xgi::Output* out);
        //action taken on submit of new HWFile
        void reloadHWFile (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //parsing of HWConfig form on submit
        void handleHWFormData (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //GUI handler for FSM transitions
        void fsmTransition (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);


      public:
        //helper to display the last page
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

        void setHWFormData (FormData* pFormData)
        {
            fHWFormData = pFormData;
        }

        void setSettingsFormData (FormData* pFormData)
        {
            fSettingsFormData = pFormData;
        }
      private:
        //determines allowed state transitions
        void createStateTransitionButton (std::string pTransition, xgi::Output* out)
        {
            // display FSM
            std::set<std::string> cPossibleInputs = fFSM->getInputs (fFSM->getCurrentState() );

            if (cPossibleInputs.find (pTransition) != std::end (cPossibleInputs) )
                *out << cgicc::input().set ("type", "submit").set ("name", "transition").set ("value", (pTransition) ) << std::endl;
            //the item is not in the list of possible transitions
            else
                *out << cgicc::input().set ("type", "submit").set ("name", "transition").set ("value", (pTransition) ).set ("disabled", "true") << std::endl;
        }

        //members
      private:
        xdaq::WebApplication* fApp;
        xgi::framework::UIManager fManager;
        log4cplus::Logger fLogger;
        std::string fURN;
        DTCStateMachine* fFSM;

      public:
        xdata::String* fHWDescriptionFile;
        xdata::String* fXLSStylesheet;
        FormData* fHWFormData;
        FormData* fSettingsFormData;
        std::string fHWFormString;
        Tab fCurrentPageView;
    };
}
#endif
