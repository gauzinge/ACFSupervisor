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
   FileName :       SupervisorGUI.h
*/

#ifndef __SupervisorGUI_H__
#define __SupervisorGUI_H__

//#include "xdaq/WebApplication.h"
#include "xdaq/Application.h"
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
#include "xdata/Integer.h"
#include "xdata/UnsignedShort.h"

#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"

#include "ACFSupervisor/ACFStateMachine.h"
#include "ACFSupervisor/FilePaths.h"

//Ph2 ACF
#include "System/SystemController.h"
#include "HWInterface/FpgaConfig.h"

#include "Utils.h"
#include "XMLUtils.h"
#include "ConsoleColor.h"

#include <chrono>
#include <thread>

using FormData = std::map<std::string, std::string>;

enum class Tab {CONFIG, MAIN, CALIBRATION, FIRMWARE, PLAYBACKDS};

namespace Ph2TkDAQ {

    class SupervisorGUI : public toolbox::lang::Class
    {
      public:
        SupervisorGUI (xdaq::Application* pApp, ACFStateMachine* pStateMachine);

        //views
        void Default (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void MainPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void ConfigPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void CalibrationPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void FirmwarePage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void PlaybackDSPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);

        ///HTML header & footer for Hyperdaq interface
        void createHtmlHeader (xgi::Input* in, xgi::Output* out, Tab pTab);
        void createHtmlFooter (xgi::Input* in, xgi::Output* out);
        ///Show FSM status and available transition in an HTML table
        void showStateMachineStatus (xgi::Output* out) throw (xgi::exception::Exception);

        // Form to load and dump HWDescription File
        void displayLoadForm (xgi::Input* in, xgi::Output* out, bool pAlone = true);
        void displayDumpForm (xgi::Input* in, xgi::Output* out);
        // Form for Ph2 ACF general settings
        void displayPh2_ACFForm (xgi::Input* in, xgi::Output* out);
        void displayPh2_ACFLog (xgi::Output* out);
        //handle dump of HWDescription file
        void dumpHWDescription (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //action taken on submit of new HWFile
        void reloadHWFile (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //process the main Ph2 ACF form
        void processPh2_ACFForm (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //parsing of HWConfig form on submit
        void handleHWFormData (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //GUI handler for FSM transitions
        void fsmTransition (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //self explanatory
        void toggleAutoRefresh (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //functions to handle FW up and download
        void loadImages (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void handleImages (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        //functions to handle the Playback and Data Sending pages
        void processDSForm (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);
        void processPlaybackForm (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception);


      public:
        //this loads the HW File from the internal string variables
        void loadHWFile();
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

                case Tab::FIRMWARE:
                    this->FirmwarePage (in, out);
                    break;

                case Tab::PLAYBACKDS:
                    this->PlaybackDSPage (in, out);
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
        std::string tailFile ()
        {
            std::string cResult = parseExternalResource (fLogFilePath.c_str() );
            return cResult;
        }

        void appendResultFiles (std::string pFile)
        {
            fResultFilesForGUI += ";" + pFile;
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

        void createProcedureInput (xgi::Output* out)
        {
            for (auto cString : fProcedures)
            {
                auto cProc = fProcedureMap.find (cString);

                if (cProc != std::end (fProcedureMap) )
                {
                    //if true
                    if (cProc->second)
                        * out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", cProc->first).set ("id", cProc->first).set ("value", "on").set ("checked", "checked") << cProc->first << cgicc::label() << cgicc::br() << std::endl;
                    else
                        * out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", cProc->first).set ("id", cProc->first).set ("value", "on") << cProc->first <<  cgicc::label() << cgicc::br() << std::endl;
                }
            }
        }

        void wait_state_changed (char& pState)
        {
            while (!isupper (pState) )
            {
                std::this_thread::sleep_for (std::chrono::milliseconds (100) );
                pState = fFSM->getCurrentState();
            }

            std::cout << "state changed to: " << pState << std::endl;
        }

        bool verifyImageName ( const std::string& strImage, const std::vector<std::string>& lstNames)
        {
            bool bFound = false;

            for (size_t iName = 0; iName < lstNames.size(); iName++)
            {
                if (!strImage.compare (lstNames[iName]) )
                {
                    bFound = true;
                    break;
                }// else cout<<strImage<<"!="<<lstNames[iName]<<endl;
            }

            if (!bFound)
                LOG4CPLUS_ERROR (fLogger, RED << "Error, this image name: " << strImage << " is not available on SD card" << RESET);

            return bFound;
        }

        void prepare_System_initialise (std::ostringstream& pLogStream)
        {
            char cState = fFSM->getCurrentState();

            if (cState != 'I')
            {
                fFSM->fireEvent ("Destroy", fApp);
                LOG4CPLUS_ERROR (fLogger, RED << "This should never happen!" << RESET);
                this->wait_state_changed (cState);
            }

            if (this->fHwXMLString.empty() )
            {
                // we haven't created the xml string yet, this normally happens on configure
                if ( this->fHWFormString.empty() )
                    this->loadHWFile();

                //now convert the HW Description HTMLString to an xml string for Initialize of Ph2ACF
                std::string cTmpFormString = cleanup_before_XSLT (this->fHWFormString);
                this->fHwXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (XMLSTYLESHEET), pLogStream, false);
                //expand all file paths from HW Description xml string
                complete_file_paths (this->fHwXMLString);
            }
        }

        //members
      private:
        xdaq::Application* fApp;
        xgi::framework::UIManager fManager;
        log4cplus::Logger fLogger;
        std::string fURN;
        ACFStateMachine* fFSM;
        const std::vector<std::string> fProcedures{"Data Taking", "Calibration", "Pedestal&Noise", "Commissioning"};
        std::string fLogFilePath;
        std::vector<std::string> fImageVector;
        std::string fResultFilesForGUI;


      public:
        xdata::UnsignedShort* fFedID;
        xdata::String* fHWDescriptionFile;
        xdata::String* fDataDirectory;
        xdata::String* fResultDirectory;
        xdata::Integer* fRunNumber;
        xdata::UnsignedInteger32* fNEvents;
        uint32_t* fEventCounter;
        xdata::Boolean* fRAWFile;
        xdata::Boolean* fDAQFile;
        std::string fHostString;
        //for the data sending
        xdata::Boolean* fSendData;
        xdata::String* fDataDestination;
        xdata::Integer* fDQMPostScale;
        xdata::String* fSourceHost;
        xdata::Integer* fSourcePort;
        xdata::String* fSinkHost;
        xdata::Integer* fSinkPort;
        //for the playback
        xdata::Boolean* fPlaybackMode;
        xdata::String* fPlaybackFile;

        FormData* fHWFormData;
        FormData* fSettingsFormData;
        std::string fHWFormString;
        std::string fSettingsFormString;
        std::string fHwXMLString;
        std::string fSettingsXMLString;

        std::map<std::string, bool> fProcedureMap;
        bool fAllChannels;
        bool fLatency;
        bool fStubLatency;
        int fLatencyStartValue;
        int fLatencyRange;

        std::string fPh2_ACFLog;
        Tab fCurrentPageView;
        bool fAutoRefresh;

        cgicc::table fDataSenderTable;
    };
}
#endif
