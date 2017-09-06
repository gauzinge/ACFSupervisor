#ifndef __DTCSupervisor_H__
#define __DTCSupervisor_H__

#include "xdaq/exception/Exception.h"
#include "xdaq/NamespaceURI.h"
#include "xdaq/WebApplication.h"

#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/task/Action.h"

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

        // the workloop, action signature and job for calibrations
        toolbox::task::ActionSignature* fCalibrationAction;
        toolbox::task::WorkLoop* fCalibrationWorkloop;
        bool CalibrationJob (toolbox::task::WorkLoop* wl);

        // the workloop, action signature and job for data taking
        toolbox::task::ActionSignature* fDAQAction;
        toolbox::task::WorkLoop* fDAQWorkloop;
        bool DAQJob (toolbox::task::WorkLoop* wl);

      protected:
        xdata::String fHWDescriptionFile;
        xdata::String fDataDirectory;
        xdata::String fResultDirectory;
        xdata::Integer fRunNumber;
        xdata::Integer fNEvents;
        xdata::Integer fEventCounter;
        xdata::Boolean fRAWFile;
        xdata::Boolean fDAQFile;

      private:
        std::string fAppNameAndInstanceString;
        FormData fHWFormData;
        FormData fSettingsFormData;
        Ph2_System::SystemController* fSystemController;
        //File Handler for SLINK Data - the one for raw data is part of SystemController
        FileHandler* fSLinkFileHandler;


      public:
        //callbacks for FSM state transtions
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

        //callbacks for FSM finished state transtions
        //run when Initialise transaction is finished
        //void Initialised (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);
        //run when Configure transaction is finished
        //void Configured (toolbox::Event::Reference e) throw (toolbox::fsm::exception::Exception);

        /** FSM call back   */
        xoap::MessageReference fsmCallback (xoap::MessageReference msg) throw (xoap::exception::Exception)
        {
            return fFSM.commandCallback (msg);
        }

      private:
        void updateHwDescription ();
        void updateSettings ();

        void handleBeBoardRegister (BeBoard* pBoard, std::map<std::string, std::string>::iterator pRegister)
        {
            std::string cKey = pRegister->first;
            cKey = removeDot (cKey.erase (0, 9) );
            uint32_t cValue = convertAnyInt (pRegister->second.c_str() );
            pBoard->setReg (cKey, cValue);

            if (fFSM.getCurrentState() == 'e')
                fSystemController->fBeBoardInterface->WriteBoardReg (pBoard, cKey, cValue);

            LOG4CPLUS_INFO (this->getApplicationLogger(), BLUE << "Changing Be Board Register " << cKey << " to " << cValue << RESET);
        }
        void handleGlobalCbcRegister (BeBoard* pBoard, std::map<std::string, std::string>::iterator pRegister)
        {
            std::string cRegName = pRegister->first.substr (pRegister->first.find ("glob_cbc_reg:") + 13 );
            uint8_t cValue = (convertAnyInt (pRegister->second.c_str() ) & 0xFF);

            for (auto cFe : pBoard->fModuleVector)
            {
                for (auto cCbc : cFe->fCbcVector)
                {
                    cCbc->setReg (cRegName, cValue);

                    if (fFSM.getCurrentState() == 'e')
                        fSystemController->fCbcInterface->WriteCbcReg (cCbc, cRegName, cValue);
                }
            }

            LOG4CPLUS_INFO (this->getApplicationLogger(), BLUE << "Changing Global CBC Register " << cRegName << " to " << +cValue << RESET);
        }
        void handleCBCRegister (BeBoard* pBoard, std::map<std::string, std::string>::iterator pRegister)
        {
            size_t cPos = pRegister->first.find ("Register_....");
            std::string cRegName = pRegister->first.substr (cPos + 13);
            cPos = cRegName.find_first_not_of ("0123456789");
            uint8_t cCbcId = (convertAnyInt (cRegName.substr (0, cPos).c_str() ) & 0xFF);
            cRegName = cRegName.substr (cPos);
            uint8_t cValue = (convertAnyInt (pRegister->second.c_str() ) & 0xFF);

            for (auto cFe : pBoard->fModuleVector)
            {
                for (auto cCbc : cFe->fCbcVector)
                {
                    if (cCbc->getCbcId() == cCbcId)
                    {
                        cCbc->setReg (cRegName, cValue);

                        if (fFSM.getCurrentState() == 'e')
                            fSystemController->fCbcInterface->WriteCbcReg (cCbc, cRegName, cValue);
                    }
                    else continue;
                }
            }

            LOG4CPLUS_INFO (this->getApplicationLogger(), BLUE << "Changing CBC Register " << cRegName << " on CBC " << +cCbcId << " to " << +cValue << RESET);
        }

        void updateLogs()
        {
            fGUI->fPh2_ACFLog.clear();
            std::string cTmpString = fGUI->tailFile () ;
            cleanup_log_string (cTmpString);
            fGUI->fPh2_ACFLog = cTmpString;
        }

    };

}

#endif
