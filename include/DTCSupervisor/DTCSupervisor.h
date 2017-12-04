/* Copyright 2017 Imperial Collge London
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Programmer :     Georg Auzinger
   Version :        1.0
   Date of creation:01/09/2017
   Support :        mail to : georg.auzinger@SPAMNOT.cern.ch
   FileName :       DTCSupervisor.h
*/

#ifndef __DTCSupervisor_H__
#define __DTCSupervisor_H__

#include "xdaq/ApplicationGroup.h"
#include "xdaq/ApplicationContext.h"
#include "xdaq/ApplicationStub.h"
#include "xdaq/Application.h"

#include "xdaq/exception/Exception.h"
#include "xdaq/NamespaceURI.h"

#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/task/Action.h"
#include "toolbox/BSem.h"
#include "toolbox/Runtime.h"

#include "xgi/Utils.h"
#include "xgi/Method.h"

#include "xoap/SOAPMessage.h"
#include "xoap/Method.h"

#include "cgicc/CgiDefs.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"

#include "xdata/ActionListener.h"
#include "xdata/String.h"
#include "xdata/Integer.h"
#include "xdata/UnsignedInteger32.h"
#include "xdata/UnsignedLong.h"
#include "xdata/UnsignedShort.h"
#include "xdata/Boolean.h"

#include "DTCSupervisor/SupervisorGUI.h"
#include "DTCSupervisor/DTCStateMachine.h"
#include "DTCSupervisor/DataSender.h"

//Ph2 ACF Includes
#include "System/SystemController.h"
#include "tools/Tool.h"
#include "tools/Calibration.h"
#include "tools/PedeNoise.h"
#include "tools/LatencyScan.h"
#include "Utils/Utilities.h"
#include "Utils/easylogging++.h"

#ifdef __HTTP__
#include "THttpServer.h"
#endif

#include <regex>

using FormData = std::map<std::string, std::string>;

namespace Ph2TkDAQ {

    //DTC Supervisor main class, responsible for GUI and user input
    class DTCSupervisor : public xdaq::Application, public xdata::ActionListener//, public xgi::framework::UIManager
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

        //soap message handler
        xoap::MessageReference fireEvent (xoap::MessageReference msg) throw (xoap::exception::Exception);

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

        // the workloop, action signature and job for Data sending to EVM
        toolbox::task::ActionSignature* fDSAction;
        toolbox::task::WorkLoop* fDSWorkloop;
        bool SendDataJob (toolbox::task::WorkLoop* wl);

        // the workloop, action signature and job for playing back data
        toolbox::task::ActionSignature* fPlaybackAction;
        toolbox::task::WorkLoop* fPlaybackWorkloop;
        bool PlaybackJob (toolbox::task::WorkLoop* wl);

      protected:
        xdata::UnsignedShort fFedID;
        xdata::String fHWDescriptionFile;
        xdata::String fDataDirectory;
        xdata::String fResultDirectory;
        xdata::Integer fRunNumber;
        xdata::UnsignedInteger32 fNEvents;
        uint32_t fEventCounter;
        xdata::Boolean fRAWFile;
        xdata::Boolean fDAQFile;
        xdata::Integer fShortPause;
        xdata::Integer fServerPort;
#ifdef __HTTP__
        THttpServer* fHttpServer;
#endif
        //for sending data to EVM
        xdata::Boolean fSendData;
        xdata::String fDataDestination;
        xdata::Integer fDQMPostScale;
        xdata::String fSourceHost;
        xdata::Integer fSourcePort;
        xdata::String fSinkHost;
        xdata::Integer fSinkPort;
        //for playback mode
        xdata::Boolean fPlaybackMode;
        xdata::String fPlaybackFile;

      private:
        toolbox::BSem fACFLock;
        std::string fAppNameAndInstanceString;
        FormData fHWFormData;
        FormData fSettingsFormData;
        Ph2_System::SystemController* fSystemController;
        //File Handler for SLINK Data - the one for raw data is part of SystemController
        FileHandler* fSLinkFileHandler;
        bool fGetRunnumberFromFile;
        Ph2TkDAQ::DataSender* fDataSender;
        uint32_t fPlaybackEventSize32;
        std::ifstream fPlaybackIfstream;


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

        std::vector<SLinkEvent> readSLinkFromFile (uint32_t pNEvents)
        {
            std::vector<SLinkEvent> cEvVec;
            bool cAnomalousEvent = false;

            for (size_t i = 0; i < pNEvents; i++)
            {
                bool cFoundTrailer = false;

                std::vector<uint64_t> cData;

                while (!cFoundTrailer && !fPlaybackIfstream.eof() )
                {
                    uint64_t cWord;
                    fPlaybackIfstream.read ( (char*) &cWord, sizeof (uint64_t) );
                    uint64_t cCorrectedWord = (cWord & 0xFFFFFFFF) << 32 | (cWord >> 32) & 0xFFFFFFFF;
                    cData.push_back (cCorrectedWord);

                    if ( (cCorrectedWord & 0xFF00000000000000) >> 56 == 0xA0 && (cCorrectedWord & 0x00FFFFFF00000000) >> 32 == cData.size() && (cCorrectedWord & 0x00000000000000F0) >> 4  == 0x7) // SLink Trailer
                    {
                        cFoundTrailer = true;
                        //break;
                    }

                    //if(fPlaybackIfstream.eof() && !cFoundTrailer)
                    //{
                    //cAnomalousEvent = true;
                    ////LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << "Error, the playback file ended but I could not find a SLink Trailer, therefore discarding theis fragment of size " << cData.size()<< RESET);
                    ////for(auto cWord : cData)
                    ////LOG4CPLUS_ERROR(this->getApplicationLogger(), std::hex << cWord << std::dec);
                    //cData.clear();
                    //break;
                    //}
                }

                //if (!cAnomalousEvent) cEvVec.push_back (SLinkEvent (cData) );
                //3 is the minimum size for an empty SLinkEvent (2 words header and 1 word trailer)
                if (cFoundTrailer && cData.size() > 3) cEvVec.push_back (SLinkEvent (cData) );

                if (fPlaybackIfstream.eof() ) break;
            }

            return cEvVec;
        }
    };

}

#endif
