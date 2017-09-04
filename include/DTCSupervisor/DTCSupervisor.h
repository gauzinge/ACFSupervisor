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

        std::vector<std::pair<std::string, uint32_t>> getBeBoardRegisters ()
        {
            std::vector<std::pair<std::string, uint32_t>> cResult;

            for (auto cPair = fHWFormData.begin(); cPair != fHWFormData.end() ; )
            {
                if (cPair->first.find ("Register_") == 0 && cPair->first.find ("...") == std::string::npos)
                {
                    std::string cKey = cPair->first;
                    cKey = removeDot (cKey.erase (0, 9 ) );
                    cResult.push_back (std::make_pair (cKey, convertAnyInt (cPair->second.c_str() ) ) );
                    LOG4CPLUS_INFO (this->getApplicationLogger(), GREEN << "Changing BeBoard Register " << cKey << " to " << cPair->second << RESET);
                    fHWFormData.erase (cPair++);
                }
                else
                    cPair++;
            }

            return cResult;
        }

        void handleCBCRegisters (BeBoard* pBoard, char pState)
        {
            for (auto cPair = fHWFormData.begin(); cPair != fHWFormData.end() ; )
            {
                if (cPair->first.find ("glob_cbc_reg") != std::string::npos)
                {
                    std::string cRegName = cPair->first.substr (cPair->first.find ("glob_cbc_reg:") );
                    uint8_t cValue = (convertAnyInt (cPair->second.c_str() ) & 0xFF);
                    fHWFormData.erase (cPair++);

                    for (auto cFe : pBoard->fModuleVector)
                    {
                        for (auto cCbc : cFe->fCbcVector)
                        {
                            cCbc->setReg (cRegName, cValue);

                            if (pState == 'e')
                                fSystemController.fCbcInterface->WriteCbcReg (cCbc, cRegName, cValue);
                        }
                    }
                }

                if (cPair->first.find ("Register_...") == 0)
                {
                    //individual CBC registers
                    std::string cRegName = cPair->first.substr (cPair->first.find ("Register_...") );
                    size_t cPos = cRegName.find_first_not_of ("0123456789");
                    uint8_t cCbcId = (convertAnyInt (cRegName.substr (0, cPos).c_str() ) & 0xFF) ;
                    cRegName = cRegName.substr (cPos);
                    uint8_t cValue = (convertAnyInt (cPair->second.c_str() ) & 0xFF);
                    fHWFormData.erase (cPair++);

                    for (auto cFe : pBoard->fModuleVector)
                    {
                        for (auto cCbc : cFe->fCbcVector)
                        {
                            if (cCbc->getCbcId() == cCbcId)
                            {
                                cCbc->setReg (cRegName, cValue);

                                if (pState == 'e')
                                    fSystemController.fCbcInterface->WriteCbcReg (cCbc, cRegName, cValue);
                            }
                            else continue;
                        }
                    }
                }

                if (cPair->first.find ("configfile") != std::string::npos)
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }
                else
                    cPair++;
            }

        }


        void handleBeBoard (BeBoard* pBoard)
        {
            for (auto cPair = fHWFormData.begin(); cPair != fHWFormData.end() ; )
            {
                if (cPair->first.find ("beboard_id") != std::string::npos)
                {
                    pBoard->setBeId (convertAnyInt (cPair->second.c_str() ) );
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first == "boardType" )
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first == "eventType" )
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first == "connection_id" )
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first == "connection_uri" )
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first == "connection_address_table" )
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first.find ("fe_id" ) != std::string::npos)
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first.find ("fmc_id" ) != std::string::npos)
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first.find ("module_id" ) != std::string::npos)
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }

                if (cPair->first.find ("module_status" ) != std::string::npos)
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Changing this is not allowed in this state!" << RESET);
                    fHWFormData.erase (cPair++);
                }
                else
                    cPair++;
            }
        }

        void handleCondtionData (BeBoard* pBoard)
        {
            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDYELLOW << "Warning, you tried to change the condition data after initialize - this is highly experimental - to be sure I would try to restart and change the condition data before initialising! - or buy a cake for Georg and ask him nicely to fix it!" << RESET);
            ConditionDataSet* pSet = pBoard->getConditionDataSet();

            auto cMode = fHWFormData.find ("debugMode");

            if (cMode != std::end (fHWFormData) )
            {
                if (cMode->second == "FULL")
                    pSet->setDebugMode (SLinkDebugMode::FULL);
                else if (cMode->second == "SUMMARY")
                    pSet->setDebugMode (SLinkDebugMode::SUMMARY);
                else if (cMode->second == "ERROR")
                    pSet->setDebugMode (SLinkDebugMode::ERROR);
                else
                    LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << "Error, SLink DEBUG mode not defined!" << RESET);

                fHWFormData.erase (cMode);
            }

            //now get the condition data item to change
            for (auto cPair = fHWFormData.begin(); cPair != fHWFormData.end();)
            {

                if (cPair->first.find ("ConditionData_") == 0 )
                {
                    //i have a condition data filed that changed type
                    size_t index = std::stoi (cPair->first.substr (cPair->first.find_last_of ("_") ) );
                    std::cout << "Cond data index: " << index << std::endl;

                    if (index - 1 < pSet->fCondDataVector.size() )
                    {
                        //now I have the item
                        if (cPair->first.length() < 16) // this is the actual type
                        {
                            //set the internal (automatic UID to the value coresponding to the form element)
                            if (cPair->second == "I2C")
                                pSet->fCondDataVector.at (index - 1).fUID = 1;
                            //else if (cPair->second == "User")
                            else if (cPair->second == "TDC")
                            {
                                pSet->fCondDataVector.at (index - 1).fUID = 3;
                                pSet->fCondDataVector.at (index - 1).fFeId = 0xFF;
                            }
                            else if (cPair->second == "HV")
                                pSet->fCondDataVector.at (index - 1).fUID = 5;
                        }

                        //these are the easy cases as I just need to change Fe or Cbc Id
                        if (cPair->first.find ("FeId") != std::string::npos)
                            pSet->fCondDataVector.at (index - 1).fFeId = std::stoi (cPair->second);
                        else if (cPair->first.find ("CbcId") != std::string::npos)
                            pSet->fCondDataVector.at (index - 1).fCbcId = std::stoi (cPair->second);
                        else if (cPair->first.find ("Value") != std::string::npos)
                            pSet->fCondDataVector.at (index - 1).fValue = std::stoi (cPair->second);
                        //in case it is HV, sensor is in the map passed from GUI and this maps to Cbc in Ph2ACF
                        else if (cPair->first.find ("Sensor") != std::string::npos)
                            pSet->fCondDataVector.at (index - 1).fCbcId = std::stoi (cPair->second);
                        //in case we are treating a user input, UID is defined
                        else if (cPair->first.find ("UID") != std::string::npos)
                            pSet->fCondDataVector.at (index - 1).fUID = std::stoi (cPair->second);

                        //this means that the type is I2C and thus I need to update the page and address
                        else if (cPair->first.find ("Register") != std::string::npos)
                        {
                            pSet->fCondDataVector.at (index - 1).fRegName = cPair->second;
                            std::string cRegName = cPair->second;
                            uint8_t cFeId = pSet->fCondDataVector.at (index - 1).fFeId;
                            uint8_t cCbcId = pSet->fCondDataVector.at (index - 1).fCbcId;
                            uint8_t cPage;
                            uint8_t cAddress;
                            uint8_t cValue;

                            for (auto cFe : pBoard->fModuleVector )
                            {
                                if (cFe->getFeId() != cFeId ) continue;

                                for (auto cCbc : cFe->fCbcVector )
                                {
                                    if (cCbc->getCbcId() != cCbcId) continue;
                                    else if (cCbc->getFeId() == cFeId && cCbc->getCbcId() == cCbcId)
                                    {
                                        CbcRegItem cRegItem = cCbc->getRegItem ( cRegName );
                                        cPage = cRegItem.fPage;
                                        cAddress = cRegItem.fAddress;
                                        cValue = cRegItem.fValue;
                                    }
                                    else
                                        LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << "SLINK ERROR: no Cbc with Id " << +cCbcId << " on Fe " << +cFeId << " - check your SLink Settings!" << RESET);
                                }
                            }

                            pSet->fCondDataVector.at (index - 1).fPage = cPage;
                            pSet->fCondDataVector.at (index - 1).fRegister = cAddress;
                            pSet->fCondDataVector.at (index - 1).fValue = cValue;
                        }
                    }

                    fHWFormData.erase (cPair++);
                }
                else
                    cPair++;
            }
        }
    };

}

#endif
