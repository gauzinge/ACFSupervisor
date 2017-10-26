#include "DTCSupervisor/SupervisorGUI.h"
#include <fstream>

using namespace Ph2TkDAQ;
using namespace cgicc;

SupervisorGUI::SupervisorGUI (xdaq::Application* pApp, DTCStateMachine* pStateMachine) :
    fApp (pApp),
    fManager (pApp),
    fFSM (pStateMachine),
    fLogFilePath (""),
    fLatency (true),
    fStubLatency (false),
    fLatencyStartValue (0),
    fLatencyRange (10),
    fHWDescriptionFile (nullptr),
    fHWFormString (""),
    fSettingsFormString (""),
    fHwXMLString (""),
    fAutoRefresh (false)
    //fSystemController (nullptr)
{
    for (auto cString : fProcedures)
        fProcedureMap[cString] = (cString == "Data Taking") ? true : false;

    fLogger = pApp->getApplicationLogger();
    fURN =  pApp->getApplicationDescriptor()->getContextDescriptor()->getURL() + "/" + pApp->getApplicationDescriptor()->getURN() + "/";

    //when I am at home
    size_t cPos = fURN.find (".cern.ch");
    fURN.erase (cPos, 8);
    std::cout << fURN << std::endl;

    //bind xgi and xoap commands to methods
    //methods for tab navigation
    //xgi::bind (this, &SupervisorGUI::MainPage, "Default");
    xgi::bind (this, &SupervisorGUI::MainPage, "MainPage");
    xgi::bind (this, &SupervisorGUI::ConfigPage, "ConfigPage");
    xgi::bind (this, &SupervisorGUI::CalibrationPage, "CalibrationPage");
    xgi::bind (this, &SupervisorGUI::FirmwarePage, "FirmwarePage");
    xgi::bind (this, &SupervisorGUI::PlaybackDSPage, "PlaybackDSPage");

    //helper methods for buttons etc
    xgi::bind (this, &SupervisorGUI::reloadHWFile, "reloadHWFile");
    xgi::bind (this, &SupervisorGUI::dumpHWDescription, "dumpHWDescription");
    xgi::bind (this, &SupervisorGUI::fsmTransition, "fsmTransition");
    xgi::bind (this, &SupervisorGUI::processPh2_ACFForm, "processPh2_ACFForm");
    xgi::bind (this, &SupervisorGUI::handleHWFormData, "handleHWFormData");
    xgi::bind (this, &SupervisorGUI::lastPage, "lastPage");
    xgi::bind (this, &SupervisorGUI::toggleAutoRefresh, "toggleAutoRefresh");
    xgi::bind (this, &SupervisorGUI::loadImages, "loadImages");
    xgi::bind (this, &SupervisorGUI::handleImages, "handleImages");
    xgi::bind (this, &SupervisorGUI::processDSForm, "processDSForm");
    xgi::bind (this, &SupervisorGUI::processPlaybackForm, "processPlaybackForm");

    fCurrentPageView = Tab::MAIN;
    fLogFilePath = (expandEnvironmentVariables ("${DTCSUPERVISOR_ROOT}/logs/DTCSuper.log") );

    if (remove (fLogFilePath.c_str() ) != 0)
        LOG4CPLUS_INFO (fLogger, BOLDRED << "Error removing Log file from previous Run: " << fLogFilePath << RESET);
}
void SupervisorGUI::toggleAutoRefresh (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    if (fAutoRefresh) fAutoRefresh = false;
    else fAutoRefresh = true;

    this->lastPage (in, out);
}

void SupervisorGUI::MainPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::string url = fURN + "reloadHWFile";
    //define view and create header
    fCurrentPageView = Tab::MAIN;
    this->createHtmlHeader (in, out, fCurrentPageView);

    // generate the page content
    *out << h3 ("DTC Supervisor Main Page") << std::endl;
    this->displayLoadForm (in, out);
    this->displayPh2_ACFForm (in, out);
    this->displayPh2_ACFLog (out);
    this->createHtmlFooter (in, out);
}

void SupervisorGUI::ConfigPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;
    //define view and create header
    fCurrentPageView = Tab::CONFIG;
    this->createHtmlHeader (in, out, fCurrentPageView);
    *out << h3 ("DTC Supervisor HwDescription Page") << std::endl;

    char cState = fFSM->getCurrentState();

    *out << table() << std::endl;
    this->displayLoadForm (in, out, false);
    this->displayDumpForm (in, out);
    *out << table() << std::endl;

    // Display the HwDescription HTML form
    std::string url = fURN + "handleHWFormData";

    *out << form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data").set ("onload", "DisplayFieldsOnload();") << std::endl;

    if (cState != 'E')
    {
        *out << input().set ("type", "submit").set ("title", "submit the entered values").set ("value", "Submit") << std::endl;
        *out << input().set ("type", "reset").set ("title", "reset the form").set ("value", "Reset") << std::endl;
    }
    else
    {
        *out << input().set ("type", "submit").set ("title", "submit the entered values").set ("value", "Submit").set ("disabled", "true") << std::endl;
        *out << input().set ("type", "reset").set ("title", "reset the form").set ("value", "Reset").set ("disabled", "true") << std::endl;
    }


    *out << fHWFormString << std::endl;
    *out << form() << std::endl;
    this->createHtmlFooter (in, out);

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::CalibrationPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;
    //define view and create header
    fCurrentPageView = Tab::CALIBRATION;
    this->createHtmlHeader (in, out, fCurrentPageView);
    *out << h3 ("DTC Supervisor Calibration Page") << std::endl;

    char cState = fFSM->getCurrentState();

#ifndef __HTTP__
    *out << span ("This feature needs ROOT built with THttpServer!").set ("style", "color:red;") << std::endl;
#else
    *out << script ().set ("type", "text/javascript").set ("src", "https://root.cern/js/latest/scripts/JSRootCore.js?more2d&io")  << script() << std::endl;
    *out << script (parseExternalResource ("html/jsplots.js", cLogStream) ).set ("type", "text/javascript") <<  std::endl;

    *out << "<div style=\"display:inline; height:90%;\">" << std::endl;
#ifdef __ROOT6__
    //*out << cgicc::div().set ("id", "simpleGUI").set ("files", "http://cmsuptrackerdaq.cern.ch:8080/Calibrations/CommissioningCycle.root") << std::endl;
    //*out << cgicc::div().set ("id", "simpleGUI").set ("files", "../../Results/CommissioningCycle_06-09-17_19:00/CommissioningCycle.root") << std::endl;
    *out << cgicc::div().set ("id", "drawing") << std::endl;
    //*out << cgicc::div().set ("id", "MainDiv").set ("style", "width:calc(100\%-250px;float:right;").set ("onload", "StatGUI();") << std::endl;
#else
    *out << iframe ().set ("src", "http://" + fHostString + "?monitoring=1000").set ("style", "width:100%;height:100%;") << std::endl;
#endif
    *out << "</div>" << std::endl;
#endif

    this->createHtmlFooter (in, out);

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::FirmwarePage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;
    //define view and create header
    fCurrentPageView = Tab::FIRMWARE;
    this->createHtmlHeader (in, out, fCurrentPageView);
    *out << h3 ("DTC Supervisor Firmware Page") << std::endl;

    char cState = fFSM->getCurrentState();

    this->displayLoadForm (in, out);

    *out << cgicc::div().set ("style", "display:inline") << std::endl;

    *out << cgicc::div() << std::endl;

    if (cState != 'I')
    {
        *out << p (span ("Firmware operations can only be done in Initial State!!").set ("style", "color:red; font-size: 20pt") ).set ("style", "margin-top: 20px") << std::endl;
        *out << p ().add (span ("Current State: ").set ("style", "color:blue; font-size: 20pt") ).add (span (fFSM->getStateName (cState) ).set ("style", "color:red; font-size: 20pt") ) << std::endl;
    }

    else
    {
        //display the form
        *out << legend ("Fw Settings").set ("style", "padding-top:20px") << fieldset() << std::endl;
        //the button / form for loading the stuff
        *out << form().set ("method", "POST").set ("action", fURN + "loadImages").set ("enctype", "multipart/form-data").add (table().set ("style", "border-spacing:10px; border-collapse: separate;").add (tr().add (td (input ().set ("type", "submit").set ("title", "load list of FW images from FC7").set ("name", "listImages").set ("value", "List Images") ) ).add (td (input ().set ("type", "submit").set ("title", "reboot the FC7").set ("name", "rebootBoard").set ("value", "Reboot Board") ) ) ) ) << std::endl;
        //the cgicc::select node to cgicc::select the image
        *out << form ().set ("method", "POST").set ("action", fURN + "handleImages").set ("enctype", "multipart/form-data") << std::endl;
        *out << "<table style=\"padding-top:20px; border-spacing:10px; border-collapse:separate;\">" << std::endl;
        *out << tr() << std::endl;
        *out << td (label ("Image List:  ") ) << td() << cgicc::select ().set ("name", "FW images") << std::endl;

        if (fImageVector.size() )
        {
            for (auto cImage : fImageVector)
                *out << option (cImage) << std::endl;
        }

        *out << cgicc::select() << td() << std::endl;
        *out << tr() << std::endl;

        *out << tr() << std::endl;
        //now put a couple of inputs(submit type there with the various actions)
        *out << td (input ().set ("type", "submit").set ("name", "loadImage").set ("title", "configure FPGA with cgicc::selected FW image").set ("value", "Load Image") ) << std::endl;
        *out << td (input ().set ("type", "submit").set ("name", "deleteImage").set ("title", "delete selected FW image").set ("value", "Delete Image") ) << std::endl;
        *out << tr() << std::endl;

        *out << tr().add (td (label ("Path to Download:  ") ) ).add (td (input ().set ("type", "text").set ("name", "pathdownload").set ("size", "50").set ("value", expandEnvironmentVariables (HOME) + "/myImage.bin" ).set ("title", "download cgicc::selected FW image") ) ) << std::endl;
        *out << tr().add (td (input ().set ("type", "submit").set ("name", "downloadImage").set ("title", "download selected FW image").set ("value", "Download") ) ).add (td (input ().set ("type", "submit").set ("name", "uploadImage").set ("title", "upload selected FW image").set ("value", "Upload") ) ) << std::endl;

        *out << tr().add (td (label ("Image to upload:  ") ) ).add (td (input ().set ("type", "file").set ("name", "Image").set ("size", "70") ) ) << std::endl;
        *out <<   tr().add (td (label ("Image Name:  ") ) ).add (td (input ().set ("type", "text").set ("name", "imagename").set ("size", "30").set ("value", "myImage.bin" ).set ("title", "name for uploaded FW image on SD Card") ) )  << std::endl;

        *out << "</table>" << std::endl;
        *out << form() << std::endl;

        *out << fieldset() << std::endl;
    }

    *out << cgicc::div() << std::endl;


    //first, we are messing with Firmware, so before we do anything, we should destroy the running configuration
    //or maybe this should happen the very second I am actually taking action
    //if (cState != 'I')
    //fFSM->fireEvent ("Destroy");

    this->createHtmlFooter (in, out);

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}
void SupervisorGUI::PlaybackDSPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;
    //define view and create header
    fCurrentPageView = Tab::PLAYBACKDS;
    this->createHtmlHeader (in, out, fCurrentPageView);
    *out << h3 ("DTC Supervisor Playback and Data Sender Page") << std::endl;

    char cState = fFSM->getCurrentState();

    this->displayLoadForm (in, out);

    *out << "<cgicc::div style=\"display: inline-block; widht=100%\">" << std::endl;

    if (cState != 'I')
    {
        *out << cgicc::div().set ("class", "acf_left") << std::endl;
        *out << p (span ("Playback and Data Sender Operations can only be configured in Initial State!!").set ("style", "color:red; font-size: 15pt") ).set ("style", "margin-top: 20px") << std::endl;
        *out << p ().add (span ("Current State: ").set ("style", "color:blue; font-size: 20pt") ).add (span (fFSM->getStateName (cState) ).set ("style", "color:red; font-size: 20pt") ) << std::endl;
        *out << cgicc::div() << std::endl;

        if (*fSendData)
        {
            *out << cgicc::div().set ("class", "acf_right") << std::endl;
            *out << cgicc::h4 ("Data Sender Stats").set ("style", "padding-top:20px;") << std::endl;
            *out << fDataSenderTable << std::endl;
            *out << cgicc::div() << std::endl;
        }
    }
    else
    {
        *out << cgicc::div().set ("class", "DS_left") << std::endl;
        //display the form
        *out << legend ("Data Sender Settings").set ("style", "padding-top:20px") << fieldset() << std::endl;
        //the button / form for loading the stuff
        *out << form().set ("method", "POST").set ("action", fURN + "processDSForm").set ("enctype", "multipart/form-data") << std::endl;
        *out << "<table style=\"padding-top:10px; border-spacing:10px; border-collapse:separate;\">" << std::endl;

        //TODO: here goes the DS configuration
        if (*fSendData)
            *out << tr().add (td (label ("Send Data") ) ).add (td (input().set ("type", "checkbox").set ("name", "send_data").set ("value", "on").set ("checked", "checked").set ("id", "Options_send") ) )  << std::endl;
        else
            *out << tr().add (td (label ("Send Data") ) ).add (td (input().set ("type", "checkbox").set ("name", "send_data").set ("value", "on").set ("id", "Options_send") ) )  << std::endl;

        *out << tr().add (td (label ("Source Host:  ") ) ).add (td (input ().set ("type", "text").set ("name", "sourcehost").set ("size", "30").set ("value", fSourceHost->toString() ).set ("title", "Data Sender Source host") ) ).add (td (label ("  Source Port:  ") ) ).add (td (input ().set ("type", "text").set ("name", "sourceport").set ("size", "6").set ("value", fSourcePort->toString() ).set ("title", "Data Sender Source port") ) ) << std::endl;

        *out << tr().add (td (label ("Destination Host:  ") ) ).add (td (input ().set ("type", "text").set ("name", "sinkhost").set ("size", "30").set ("value", fSinkHost->toString() ).set ("title", "Data Sender Destination host") ) ).add (td (label ("  Destination Port:  ") ) ).add (td (input ().set ("type", "text").set ("name", "sinkport").set ("size", "6").set ("value", fSinkPort->toString() ).set ("title", "Data Sender Destination port") ) ) << std::endl;

        *out << "</table>" << std::endl;
        *out << input ().set ("type", "submit").set ("title", "Submit Data Sender Settings").set ("name", "sumit_DS").set ("value", "Submit") << std::endl;
        *out << form() << std::endl;
        *out << fieldset() << std::endl;
        *out << legend ("Playback Settings").set ("style", "padding-top:20px") << fieldset() << std::endl;
        *out << form().set ("method", "POST").set ("action", fURN + "processPlaybackForm").set ("enctype", "multipart/form-data") << std::endl;
        *out << "<table style=\"padding-top:10px; border-spacing:10px; border-collapse:separate;\">" << std::endl;
        //TODO: here goes the Playback configuration

        if (*fPlaybackMode)
            *out << tr().add (td (label ("PlaybackMode:  ") ) ).add (td (input ().set ("type", "checkbox").set ("name", "playbackmode").set ("title", "Playback Mode").set ("value", "on").set ("checked", "checked") ) ).add (td (label ("  Playback File:  ") ) ).add (td (input ().set ("type", "text").set ("name", "playbackfile").set ("size", "70").set ("value", fPlaybackFile->toString() ).set ("title", "Playback Data File") ) ) << std::endl;
        else
            *out << tr().add (td (label ("PlaybackMode:  ") ) ).add (td (input ().set ("type", "checkbox").set ("name", "playbackmode").set ("title", "Playback Mode").set ("value", "on") ) ).add (td (label ("  Playback File:  ") ) ).add (td (input ().set ("type", "text").set ("name", "playbackfile").set ("size", "50").set ("value", fPlaybackFile->toString() ).set ("title", "Playback Data File") ) ) << std::endl;

        *out << "</table>" << std::endl;
        *out << input ().set ("type", "submit").set ("title", "Submit Playback Settings").set ("name", "sumit_playback").set ("value", "Submit") << std::endl;
        *out << form() << std::endl;
        *out << fieldset() << std::endl;
        *out << cgicc::div() << std::endl;

        if (*fSendData)
        {
            *out << cgicc::div().set ("class", "DS_right") << std::endl;
            *out << cgicc::h4 ("Data Sender Stats").set ("style", "padding-top:20px;") << std::endl;
            *out << fDataSenderTable << std::endl;
            *out << cgicc::div() << std::endl;
        }
    }

    *out << "</div>" << std::endl;
    this->createHtmlFooter (in, out);

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::createHtmlHeader (xgi::Input* in, xgi::Output* out, Tab pTab)
{
    //draw the xdaq header
    fManager.getHTMLHeader (in, out);
    // Create the Title, Tab bar
    std::ostringstream cLogStream;
    char cState = fFSM->getCurrentState();

    //Style this thing
    *out << style (parseExternalResource (expandEnvironmentVariables (CSSSTYLESHEET), cLogStream) ) << std::endl;
    // some javascript
    std::string JSfile = expandEnvironmentVariables (HOME);

    bool cAutoRefresh = false;
    int cRefreshDelay = 3;

    if (cState == 'E' || fAutoRefresh)
        //if (fAutoRefresh)
        cAutoRefresh = true;

    std::ostringstream cAutoRefreshString;
    cAutoRefreshString << cRefreshDelay << "; " << fURN;

    std::ostringstream cTabBarString;

    // switch to show the current tab
    switch (pTab)
    {
        case Tab::MAIN:
            cTabBarString << a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button active") << a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button") << a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button") << a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button") << a ("Playback & DS Page").set ("href", fURN + "PlaybackDSPage").set ("class", "button") ;
            JSfile += "/html/formfields.js";
            cAutoRefreshString << "MainPage";
            break;

        case Tab::CONFIG:
            cTabBarString << a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button") << a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button active") << a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button") << a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button") << a ("Playback & DS Page").set ("href", fURN + "PlaybackDSPage").set ("class", "button");
            JSfile += "/html/HWForm.js";
            cAutoRefreshString << "ConfigPage";
            break;

        case Tab::CALIBRATION:
            cTabBarString << a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button") << a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button") << a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button active") << a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button") << a ("Playback & DS Page").set ("href", fURN + "PlaybackDSPage").set ("class", "button");
            JSfile += "/html/empty.js";
            cAutoRefreshString << "CalibrationPage";
            //*out << script ().set ("type", "text/javascript").set ("src", "https://root.cern/js/latest/scripts/JSRootCore.js?more2d&gui&io") << std::endl;
            //*out << script ().set ("type", "text/javascript").set ("src", "http://" + fHostString + "/jsrootsys/scripts/JSRootCore.js?more2d&io&gui")  << script() << std::endl;
            break;

        case Tab::FIRMWARE:
            cTabBarString << a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button") << a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button") << a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button") << a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button active") << a ("Playback & DS Page").set ("href", fURN + "PlaybackDSPage").set ("class", "button");
            JSfile += "/html/empty.js";
            cAutoRefreshString << "FirmwarePage";
            break;

        case Tab::PLAYBACKDS:
            cTabBarString << a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button") << a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button") << a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button") << a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button") << a ("Playback & DS Page").set ("href", fURN + "PlaybackDSPage").set ("class", "button active");
            JSfile += "/html/empty.js";
            cAutoRefreshString << "PlaybackDSPage";
            break;
    }

    //if(!cAutoRefresh)

    if (cAutoRefresh)
    {
        *out << meta().set ("HTTP-EQUIV", "Refresh").set ("CONTENT", cAutoRefreshString.str() ) << std::endl;
        cTabBarString << a ("AutoRefresh Off").set ("href", fURN + "toggleAutoRefresh" ).set ("class", "refresh_button_active") << std::endl;
    }
    else
        cTabBarString << a ("AutoRefresh On").set ("href", fURN + "toggleAutoRefresh" ).set ("class", "refresh_button") << std::endl;

    //*out << " <meta HTTP-EQUIV=\"Refresh\" CONTENT=\"" << cRefreshDelay << "; " << fURN << "MainPage\"/>" << std::endl;

    if (!JSfile.empty() ) *out << script (parseExternalResource (JSfile, cLogStream) ).set ("type", "text/javascript") << std::endl;

    *out << cgicc::div (h1 (a ("DTC Supervisor FedID " + fFedID->toString() ).set ("href", fURN + "MainPage") ) ).set ("class", "title") << std::endl;
    *out << cgicc::div (cTabBarString.str() ).set ("class", "tab") << std::endl;
    *out << "<cgicc::div class=\"main\">" << std::endl;
    this->showStateMachineStatus (out);
    *out << "<cgicc::div class=\"content\">" << std::endl;

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::createHtmlFooter (xgi::Input* in, xgi::Output* out)
{
    //close main and content cgicc::divs
    *out << "</cgicc::div>" <<  std::endl << "</cgicc::div>" << std::endl;
    //draw footer
    fManager.getHTMLFooter (in, out);
}

void SupervisorGUI::showStateMachineStatus (xgi::Output* out) throw (xgi::exception::Exception)
{
    // create the FSM Status bar showing the current state
    try
    {
        //display the sidenav with the FSM controls
        char cState = fFSM->getCurrentState();
        std::stringstream cCurrentState;
        cCurrentState << "Current State: " << fFSM->getStateName (cState) << "/" << cState << std::endl;

        *out << "<cgicc::div class=\"sidenav\">" << std::endl;
        *out << p (cCurrentState.str() ).set ("class", "state") << std::endl;
        *out << p (std::string (1, cState) ).set ("style", "display:none").set ("id", "state") << std::endl;

        //Event counter
        *out << cgicc::div().set ("class", "eventcount") << std::endl;
        *out << table() << std::endl;
        *out << tr().add (td ("Requested Events: ") ).add (td (fNEvents->toString() ) ) << std::endl;
        *out << tr().add (td ("Event Count: ") ).add (td (std::to_string (*fEventCounter ) ) ) << std::endl;
        *out << table() << std::endl;
        *out << cgicc::div()  << std::endl;

        // display FSM
        // I could loop all possible transitions and check if they are allowed but that does not put the commands sequentially
        // https://gitlab.cern.ch/cms_tk_ph2/BoardSupervisor/blob/master/src/common/BoardSupervisor.cc

        //the action is the fsmTransitionHandler
        std::string url = fURN + "fsmTransition";
        *out << form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data") << std::endl;
        //first the refresh button
        *out << input().set ("type", "submit").set ("name", "transition").set ("value", "Refresh" ).set ("class", "refresh") << std::endl;
        this->createStateTransitionButton ("Initialise", out);
        this->createStateTransitionButton ("Configure", out);
        this->createStateTransitionButton ("Enable", out);
        this->createStateTransitionButton ("Stop", out);
        this->createStateTransitionButton ("Pause", out);
        this->createStateTransitionButton ("Resume", out);
        this->createStateTransitionButton ("Halt", out);
        this->createStateTransitionButton ("Destroy", out);
        *out << form() << std::endl;

        std::string cFilename = fHWDescriptionFile->toString().substr (fHWDescriptionFile->toString().find_last_of ("/") + 1);
        *out << cgicc::div ("Current HW File: " + cFilename).set ("class", "current_file") << std::endl;
        *out << "</cgicc::div>" << std::endl;
    }
    catch (xgi::exception::Exception& e)
    {
        XCEPT_RETHROW (xgi::exception::Exception, "Exception caught in WebShowRun", e);
    }
}

void SupervisorGUI::fsmTransition (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;
    char cState = fFSM->getCurrentState();

    try
    {
        //get the state transition tirggered by the button in the sidebar and propagate to the main DTC supervisor application
        //before take action as required by the state transition on any GUI elements
        Cgicc cgi (in);
        std::string cTransition = cgi["transition"]->getValue();

        //here handle whatever action is necessary on the gui before handing over to the main application
        //this should be done in DTCSupervisor::initialising() since otherwise it will not work with RCMS
        //if (cTransition == "Initialise" )
        //{
            //if ( fHWFormString.empty() )
                //this->loadHWFile();

            ////now convert the HW Description HTMLString to an xml string for Initialize of Ph2ACF
            //std::string cTmpFormString = cleanup_before_XSLT (fHWFormString);
            //fHwXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (XMLSTYLESHEET), cLogStream, false);
            ////now do the same for the Settings
            //cTmpFormString = cleanup_before_XSLT_Settings (fSettingsFormString);
            //fSettingsXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (SETTINGSSTYLESHEETINVERSE), cLogStream, false);
        //}

        if (cTransition == "Refresh")
        {
            this->lastPage (in, out);
            return;
        }
        else
        {
            fFSM->fireEvent (cTransition, fApp);
            //this should now be the transition state
            cState = fFSM->getCurrentState();
        }
    }
    catch (const std::exception& e)
    {
        XCEPT_RAISE (xgi::exception::Exception, e.what() );
    }

    //here just wait for the state change in a blocking fashion - we don't care
    this->wait_state_changed (cState);
    this->lastPage (in, out);

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}


void SupervisorGUI::displayLoadForm (xgi::Input* in, xgi::Output* out, bool pAlone)
{
    //get FSM state
    char cState = fFSM->getCurrentState();

    std::string url = fURN + "reloadHWFile";

    //*out << cgicc::div().set ("padding", "10px") << std::endl;

    //only allow changing the HW Description File in state initial
    *out << form().set ("padding", "10px").set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data").set ("autocomplete", "on") << std::endl;

    if (pAlone)
        *out << "<table>" << std::endl;

    if (cState == 'I')
        *out << tr().add (td (label ("Hw Description File Path:").set ("for", "HwDescriptionFile") ) ).add (td (input().set ("type", "text").set ("name", "HwDescriptionFile").set ("id", "HwDescriptionFile").set ("size", "70").set ("value", fHWDescriptionFile->toString() ) ) ).add (td (input().set ("type", "submit").set ("id", "hwForm_load_submit").set ("title", "change the Hw Description File").set ("value", "Reload") ) ) << std::endl;
    else
        *out << tr().add (td (label ("Hw Description File Path:").set ("for", "HwDescriptionFile") ) ).add (td (input().set ("type", "text").set ("name", "HwDescriptionFile").set ("id", "HwDescriptionFile").set ("size", "70").set ("value", fHWDescriptionFile->toString() ).set ("disabled", "disabled")  ) ).add (td (input().set ("type", "submit").set ("id", "hwForm_load_submit").set ("title", "change the Hw Description File").set ("value", "Reload").set ("disabled", "disabled") ) ) << std::endl;

    if (pAlone)
        *out << "</table>" << std::endl;

    *out << form() << std::endl;
}

void SupervisorGUI::reloadHWFile (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    // stream for logger
    std::ostringstream cLogStream;
    std::string cHWDescriptionFile;

    try
    {
        //parse the form input
        Cgicc cgi (in);
        form_iterator cIt = cgi.getElement ("HwDescriptionFile");

        if (!cIt->isEmpty() && cIt != (*cgi).end() )
            cHWDescriptionFile = cIt->getValue();

        //if (Ph2TkDAQ::checkFile (cHWDescriptionFile) )
        if (!cHWDescriptionFile.empty() )
        {
            *fHWDescriptionFile = cHWDescriptionFile;
            cLogStream << BLUE << std::endl << "Changed HW Description File to: " << fHWDescriptionFile->toString() << RESET << std::endl;
        }

        //else
        //LOG4CPLUS_ERROR (fLogger, "Error, HW Description File " << cHWDescriptionFile << " is an empty string or does not exist! inner" );
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (fLogger, BOLDYELLOW << e.what() << RESET );
    }

    this->loadHWFile();

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );

    this->lastPage (in, out);
}

void SupervisorGUI::loadHWFile()
{
    std::ostringstream cLogStream;

    if (!fHWDescriptionFile->toString().empty() && checkFile (fHWDescriptionFile->toString() ) )
    {
        fHWFormString.clear();
        fHWFormString = XMLUtils::transformXmlDocument (fHWDescriptionFile->toString(), expandEnvironmentVariables (XSLSTYLESHEET), cLogStream);
        cleanup_after_XSLT (fHWFormString);

        fSettingsFormString.clear();
        fSettingsFormString = XMLUtils::transformXmlDocument (fHWDescriptionFile->toString(), expandEnvironmentVariables (SETTINGSSTYLESHEET), cLogStream);
        cleanup_after_XSLT_Settings (fSettingsFormString);

        if (fSettingsFormString.empty() )
            LOG4CPLUS_ERROR (fLogger, RED << "Error, HW Description File " << fHWDescriptionFile->toString() << " does not contain any run settings!" << RESET);
    }
    else
        LOG4CPLUS_ERROR (fLogger, RED << "Error, HW Description File " << fHWDescriptionFile->toString() << " is an empty string or does not exist!" << RESET);

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}


void SupervisorGUI::displayDumpForm (xgi::Input* in, xgi::Output* out)
{
    //string defining action
    std::string url = fURN + "dumpHWDescription";
    *out << form().set ("style", "padding-top:10px").set ("style", "padding-bottom:10px").set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data") << std::endl;
    *out << tr().add (td (label ("Path for Hw File dump: ").set ("for", "fileDumpPath") ) ).add (td (input().set ("type", "text").set ("name", "fileDumpPath").set ("title", "file path to dump HW Description File").set ("value", expandEnvironmentVariables (HOME) ).set ("size", "70") ) ).add (td (input().set ("type", "submit").set ("title", "dump HWDescription form to XML File").set ("value", "Dump") ) ) << std::endl;
    *out << form() << std::endl;
}

void SupervisorGUI::dumpHWDescription (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    //check if the form string is empty
    if ( fHWFormString.empty() )
        LOG4CPLUS_ERROR (fLogger, RED << "Error: HW description file has not been loaded - thus no changes to dump - refer to the original file!" << RESET);
    //this->reloadHWFile (in, out);
    else
    {
        try
        {
            std::string cFileDumpPath;
            //parse the form input
            Cgicc cgi (in);
            form_iterator cIt = cgi.getElement ("fileDumpPath");

            if (!cIt->isEmpty() && cIt != (*cgi).end() )
            {
                cFileDumpPath = cIt->getValue();

                if (cFileDumpPath.back() != '/')
                    cFileDumpPath += "/";

                std::string cFilename = fHWDescriptionFile->toString().substr (fHWDescriptionFile->toString().find_last_of ("/") + 1);
                cFileDumpPath += cFilename;

                //now convert the HTMLString to an xml string for Initialize of Ph2ACF
                std::string cTmpFormString = cleanup_before_XSLT (fHWFormString);
                std::ostringstream cLogStream;
                std::string cHwXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (XMLSTYLESHEET), cLogStream, false);
                std::ofstream cOutFile (cFileDumpPath.c_str() );
                cOutFile << cHwXMLString << std::endl;
                cOutFile.close();

                if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
            }
            else
                LOG4CPLUS_ERROR (fLogger, RED << "Error: no path provided to dump to!" << RESET);
        }
        catch (const std::exception& e)
        {
            LOG4CPLUS_ERROR (fLogger, RED << e.what() << RESET );
        }
    }
}

void SupervisorGUI::displayPh2_ACFForm (xgi::Input* in, xgi::Output* out)
{
    //get FSM state
    char cState = fFSM->getCurrentState();
    std::ostringstream cLogStream;

    std::string url = fURN + "processPh2_ACFForm";

    *out << "<cgicc::div style=\"display: inline-block; widht=100%\">" << std::endl;

    *out << form().set ("id", "acfform").set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data").set ("onclick", "displayACFfields();") << std::endl;

    //left form part
    *out << cgicc::div().set ("class", "acf_left") << std::endl;

    //Procedure settings
    if (cState == 'E' || cState == 'C' )
        *out << fieldset().set ("id", "procedurelist").set ("style", "margin-top:20px").set ("onblur", "submitACFForm();").set ("disabled", "disabled") << legend ("Ph2_ACF Main Settings") << std::endl;
    else
        *out << fieldset().set ("id", "procedurelist").set ("style", "margin-top:20px").set ("onblur", "submitACFForm();") << legend ("Ph2_ACF Main Settings") << std::endl;

    this->createProcedureInput (out);
    *out << fieldset() << std::endl;
    //end of procedure settings

    //Main Settings parsed from file!
    if (cState == 'E')
        *out << fieldset().set ("style", "margin-top:10px; display:none").set ("id", "settings_fieldset") << legend ("Application Settings").set ("id", "settings_fieldset_legend").set ("style", "display:none").set ("disabled", "disabled") << std::endl;
    else
        *out << fieldset().set ("style", "margin-top:10px; display:none").set ("id", "settings_fieldset") << legend ("Application Settings").set ("id", "settings_fieldset_legend").set ("style", "display:none") << std::endl;

    *out << fSettingsFormString << std::endl;
    *out << input().set ("type", "button").set ("value", "Add Setting").set ("onClick", "addSetting('settings_table');") << std::endl;
    *out << fieldset() << std::endl;
    //end of Main Settings
    *out << cgicc::div() << std::endl;

    //right form part
    *out << cgicc::div().set ("class", "acf_right") << std::endl;

    //main run settings
    *out << fieldset().set ("style", "margin-top:20px") << legend ("Main Run Settings") << std::endl;
    *out << table() << std::endl;
    *out << tr() << std::endl;

    if (cState == 'I' || cState == 'H')
    {
        if (*fRAWFile)
            *out << td().add (label ("Write RAW File") ).add (input().set ("type", "checkbox").set ("name", "raw_file").set ("value", "on").set ("checked", "checked").set ("id", "fileOptions_raw") ) << std::endl;
        else
            *out << td().add (label ("Write RAW File") ).add (input().set ("type", "checkbox").set ("name", "raw_file").set ("value", "on").set ("id", "fileOptions_raw") ) << std::endl;

        if (*fDAQFile)
            *out << td().add (label ("Write DAQ File") ).add (input().set ("type", "checkbox").set ("name", "daq_file").set ("value", "on").set ("checked", "checked").set ("id", "fileOptions_daq") )  << std::endl;
        else
            *out << td().add (label ("Write DAQ File") ).add (input().set ("type", "checkbox").set ("name", "daq_file").set ("value", "on").set ("id", "fileOptions_daq") )  << std::endl;

        *out << tr() << tr() << std::endl;

        if (*fSendData)
            *out << td().add (label ("Send Data") ).add (input().set ("type", "checkbox").set ("name", "send_data").set ("value", "on").set ("checked", "checked").set ("id", "Options_send") )  << std::endl;
        else
            *out << td().add (label ("Send Data") ).add (input().set ("type", "checkbox").set ("name", "send_data").set ("value", "on").set ("id", "Options_send") )  << std::endl;

        *out << tr() << std::endl;
    }
    else
    {
        if (*fRAWFile)
            *out << td().add (label ( "Write RAW File" ) ).add (input().set ("type", "checkbox").set ("name", "raw_file").set ("value", "on").set ("checked", "checked").set ("id", "fileOptions_raw").set ("disabled", "disabled") ) <<  std::endl;
        else
            *out << td().add (label ( "Write RAW File" ) ).add (input().set ("type", "checkbox").set ("name", "raw_file").set ("value", "on").set ("id", "fileOptions_raw").set ("disabled", "disabled") ) <<  std::endl;

        if (*fDAQFile)
            *out << td().add (label ("Write DAQ File" ) ).add (input().set ("type", "checkbox").set ("name", "daq_file").set ("value", "on").set ("checked", "checked").set ("id", "fileOptions_daq").set ("disabled", "disabled") ) << std::endl;
        else
            *out << td().add (label ("Write DAQ File" ) ).add (input().set ("type", "checkbox").set ("name", "daq_file").set ("value", "on").set ("id", "fileOptions_daq").set ("disabled", "disabled") ) << std::endl;

        //since I need to open the socket on configure, I really only get to choose the send data option when state initial or halted
        *out << tr() << tr() << std::endl;

        if (*fSendData)
            *out << td().add (label ("Send Data") ).add (input().set ("type", "checkbox").set ("name", "send_data").set ("value", "on").set ("checked", "checked").set ("id", "Options_send").set ("disabled", "disabled") )  << std::endl;
        else
            *out << td().add (label ("Send Data") ).add (input().set ("type", "checkbox").set ("name", "send_data").set ("value", "on").set ("id", "Options_send").set ("disabled", "disabled") )  << std::endl;

        *out << tr() << std::endl;
    }


    if (cState == 'I' || cState == 'H')
        *out << tr().add (td (label ("Runnumber: ") ) ).add (td (input().set ("type", "text").set ("name", "runnumber").set ("size", "20").set ("value", fRunNumber->toString() ).set ("id", "runnumber") ) ) << std::endl;
    else
        *out << tr().add (td (label ("Runnumber: ") ) ).add (td (input().set ("type", "text").set ("name", "runnumber").set ("size", "20").set ("value", fRunNumber->toString() ).set ("id", "runnumber").set ("disabled", "disabled") ) ) << std::endl;


    if (cState == 'E')
        *out << tr().add (td (label ("Number of Events: ") ) ).add (td (input().set ("type", "text").set ("name", "nevents").set ("size", "20").set ("placeholder", "number of events to take").set ("value", fNEvents->toString() ).set ("id", "nevents").set ("disabled", "disabled") ) ) << std::endl;
    else
        *out << tr().add (td (label ("Number of Events: ") ) ).add (td (input().set ("type", "text").set ("name", "nevents").set ("size", "20").set ("placeholder", "number of events to take").set ("value", fNEvents->toString() ).set ("id", "nevents") ) ) << std::endl;


    if (cState == 'I' || cState == 'H')
        *out << tr().add (td (label ("Data Directory: ") ) ).add (td (input().set ("type", "text").set ("name", "data_directory").set ("size", "60").set ("value", expandEnvironmentVariables (fDataDirectory->toString() ) ).set ("id", "datadir") ) ) << std::endl;
    else
        *out << tr().add (td (label ("Data Directory: ") ) ).add (td (input().set ("type", "text").set ("name", "data_directory").set ("size", "60").set ("value", expandEnvironmentVariables (fDataDirectory->toString() ) ).set ("id", "datadir").set ("disabled", "disabled") ) ) << std::endl;


    if (cState == 'I' || cState == 'H')
        *out << tr().add (td (label ("Result Directory: ").set ("style", "display:none").set ("id", "result_directory_label") ) ).add (td (input().set ("type", "text").set ("name", "result_directory").set ("id", "result_directory").set ("size", "60").set ("value", expandEnvironmentVariables (fResultDirectory->toString() ) ).set ("style", "display:none").set ("id", "resultdir") ) ) << std::endl;
    else
        *out << tr().add ( td (label ("Result Directory: ").set ("style", "display:none").set ("id", "result_directory_label") ) ).add (td (input().set ("type", "text").set ("name", "result_directory").set ("id", "result_directory").set ("size", "60").set ("value", expandEnvironmentVariables (fResultDirectory->toString() ) ).set ("style", "display:none").set ("id", "resultdir").set ("disabled", "disabled") ) )  << std::endl;

    *out << table() << std::endl;
    *out << fieldset() << std::endl;
    //end of main run settings

    // commissioning settings
    if (cState == 'E')
        *out << fieldset().set ("id", "commission_fieldset").set ("style", "margin:10px").set ("disabled", "disabled") << legend ("Commissioning Settings").set ("id", "commission_legend") << std::endl;
    else
        *out << fieldset().set ("id", "commission_fieldset").set ("style", "margin:10px") << legend ("Commissioning Settings").set ("id", "commission_legend") << std::endl;

    //Commissioning Settings
    *out << table().set ("style", "display:none").set ("id", "commission_table") << std::endl;
    *out << tr() << std::endl;

    if (fLatency)
        *out << td().add (label ("Latency Scan") ).add (input().set ("type", "checkbox").set ("name", "latency").set ("value", "on").set ("checked", "checked") )  << std::endl;
    else
        *out << td().add (label ("Latency Scan") ).add (input().set ("type", "checkbox").set ("name", "latency").set ("value", "on") )  << std::endl;

    if (fStubLatency)
        *out << td().add (label ("Stub Latency Scan") ).add (input().set ("type", "checkbox").set ("name", "stublatency").set ("value", "on").set ("checked", "checked") ) << std::endl;
    else
        *out << td().add (label ("Stub Latency Scan") ).add (input().set ("type", "checkbox").set ("name", "stublatency").set ("value", "on") ) << std::endl;

    *out << tr() << std::endl;

    *out << tr() << std::endl;
    *out << td().add (label ("Minimum Latency: ") ).add (input().set ("type", "text").set ("name", "minimum_latency").set ("size", "5").set ("value", inttostring (fLatencyStartValue) ) ) << std::endl;
    *out << td().add (label ("Latency Range: ") ).add (input().set ("type", "text").set ("name", "latency_range").set ("size", "5").set ("value", inttostring (fLatencyRange) ) ) << std::endl;
    *out << tr() << std::endl;
    *out << table() << std::endl;
    *out << fieldset() << std::endl;
    //end of commissioning settings

    //*out << fieldset() << std::endl;

    if (cState != 'E')
        *out << input().set ("type", "submit").set ("name", "Submit").set ("id", "main_submit").set ("value", "Submit") << std::endl;
    else
        *out << input().set ("type", "submit").set ("name", "Submit").set ("value", "Submit").set ("disabled", "disabled") << std::endl;

    *out << cgicc::div() << std::endl;
    //end of right cgicc::div
    *out << form() << std::endl;
    //end of overal form
    *out << "</cgicc::div>" << std::endl;
    //end of main inline-block cgicc::div

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::processPh2_ACFForm (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    // stream for logger
    std::ostringstream cLogStream;

    try
    {
        std::vector<std::pair<std::string, std::string>> cSettingFormPairs;
        //parse the form input
        Cgicc cgi (in);

        for (auto cProc : fProcedures)
        {
            bool cOn = cgi.queryCheckbox (cProc);
            fProcedureMap[cProc] = cOn;
        }

        *fRAWFile = cgi.queryCheckbox ("raw_file");
        *fDAQFile = cgi.queryCheckbox ("daq_file");
        *fSendData = cgi.queryCheckbox ("send_data");

        fLatency = cgi.queryCheckbox ("latency");
        fStubLatency = cgi.queryCheckbox ("stublatency");

        for (auto cIt : *cgi)
        {
            if (cIt.getValue() != "")
            {

                if (cIt.getName() == "runnumber") *fRunNumber = atoi (cIt.getValue().c_str() ); //atoi since I have to potentially deal with negative numbers
                else if (cIt.getName() == "nevents") *fNEvents = stringtoint (cIt.getValue().c_str() );
                else if (cIt.getName() == "data_directory") *fDataDirectory = cIt.getValue();
                else if (cIt.getName() == "result_directory") *fResultDirectory = cIt.getValue();
                else if (cIt.getName() == "minimum_latency") fLatencyStartValue = stringtoint (cIt.getValue().c_str() );
                else if (cIt.getName() == "latency_range") fLatencyRange = stringtoint (cIt.getValue().c_str() );
                else if (cIt.getName() == "Submit" || cIt.getValue() == "on") continue; //if the input is the submit button or one of the checkboxes I already queried
                else
                {
                    //this is setting data
                    cSettingFormPairs.push_back (std::make_pair (cIt.getName(), cIt.getValue() ) );
                }
            }
        }

        //last argument stripps the unchanged fields
        //this method needs to learn to add a node in case it is not found in the document
        FormData cSettingsFormData = XMLUtils::updateHTMLForm (this->fSettingsFormString, cSettingFormPairs, cLogStream, false );

        //now I have settings form Data but Ideally I want to change it to key-value pairs containing setting:value
        if (fFSM->getCurrentState() != 'I')
        {
            if (cSettingsFormData.size() % 2 == 0)
            {
                for (auto cPair : cSettingsFormData)
                {
                    if (cPair.first.length() < 11 ) // the name of the field is "setting_%d" so I have a new key
                    {
                        size_t pos = cPair.first.find ("_");
                        std::string cKey = "setting_value_" + cPair.first.substr (pos + 1, cPair.first.length() - pos);

                        auto cValuePair = cSettingsFormData.find (cKey);

                        if (cValuePair == std::end (cSettingsFormData) )
                            LOG4CPLUS_ERROR (fLogger, RED << "Error, could not find the value for key " << cPair.second << RESET );
                        else
                            (*fSettingsFormData) [cPair.second] = cValuePair->second;
                    }
                }
            }
            else
                LOG4CPLUS_ERROR (fLogger, RED << "Error, settings map parsed from the input form has the wrong size!" << RESET );
        }
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (fLogger, RED << e.what() << RESET );
    }

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );

    this->lastPage (in, out);
}

void SupervisorGUI::displayPh2_ACFLog (xgi::Output* out)
{
    *out << cgicc::div().set ("class", "separator") << cgicc::div() << std::endl;
    *out << cgicc::div (legend ("Ph2_ACF Logs") ).set ("class", "legend") << std::endl;
    *out << cgicc::div (p (fPh2_ACFLog) ).set ("class", "log").set ("id", "logwindow") << std::endl;
}

void SupervisorGUI::handleHWFormData (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    //stream for logger
    std::ostringstream cLogStream;

    //vector of pair of string to hold values
    std::vector<std::pair<std::string, std::string>> cHWFormPairs;

    // get the form input
    try
    {
        Cgicc cgi (in);

        for (auto cIt : *cgi)
        {
            if (cIt.getValue() != "")
                cHWFormPairs.push_back (std::make_pair (cIt.getName(), cIt.getValue() ) );
        }

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (fLogger, RED << e.what() << RESET );
    }

    //always strip the unchanged since I am only using this info in the fHWFormData to update once the HWTree is initialized
    *fHWFormData = XMLUtils::updateHTMLForm (this->fHWFormString, cHWFormPairs, cLogStream, true );

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );

    this->lastPage (in, out);
}

void SupervisorGUI::loadImages (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;

    // get the form input
    bool cList = false;
    bool cReboot = false;

    try
    {
        Cgicc cgi (in);

        for (auto cIt : *cgi)
        {
            if (cIt.getValue() != "")
            {
                if (cIt.getValue() == "List Images") cList = true;
                else if (cIt.getValue() == "Reboot Board") cReboot = true;
            }
        }
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (fLogger, RED << e.what() << RESET );
    }

    try
    {

        Ph2_System::SystemController fSystemController;

        this->prepare_System_initialise (cLogStream);
        fSystemController.InitializeHw (this->fHwXMLString, cLogStream, false);
        BeBoard* pBoard = fSystemController.fBoardVector.at (0);

        if (cList)
        {
            fImageVector.clear();
            fImageVector = fSystemController.fBeBoardInterface->getFpgaConfigList (pBoard);
        }
        else if (cReboot)
        {
            LOG4CPLUS_INFO (fLogger, GREEN << "Rebooting Board - this will cause a segfault and needs a restart of the application!" << RESET);
            fSystemController.fBeBoardInterface->RebootBoard (pBoard);
        }

        fSystemController.Destroy();

    }
    catch (const std::exception& e)
    {
        XCEPT_RAISE (xgi::exception::Exception, e.what() );
    }

    this->lastPage (in, out);
}

void SupervisorGUI::handleImages (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    // get the form input
    bool cLoad = false;
    bool cDelete = false;
    bool cDownload = false;
    bool cUpload = false;
    std::string cDownloadPath = "";
    std::string cImagePath = "";
    std::string cImageName = "";
    std::string cImage = "";

    try
    {
        Cgicc cgi (in);

        for (auto cIt : *cgi)
        {
            std::cout << cIt.getName() << " " << cIt.getValue() << std::endl;

            if (cIt.getValue() == "Load Image") cLoad = true;

            if (cIt.getValue() == "Delete Image") cDelete = true;

            if (cIt.getValue() == "Upload") cUpload = true;

            if (cIt.getValue() == "Download") cDownload = true;
        }

        if (cLoad || cDelete || cDownload)
        {
            form_iterator cIt = cgi.getElement ("FW Images");

            if (!cIt->isEmpty() && cIt != (*cgi).end() )
            {
                cImage = cIt->getValue();
                LOG4CPLUS_INFO (fLogger, GREEN << "Changing to Image " << cImage << " this will cause a segfault and needs a restart of the application!" << RESET);

                if (!this->verifyImageName (cImage, fImageVector) )
                {
                    this->lastPage (in, out);
                    LOG4CPLUS_ERROR (fLogger, RED << "The image is not on the SD card!" << RESET);
                    return;
                }
            }
        }

        if (cDownload)
        {
            form_iterator cIt = cgi.getElement ("pathdownload");

            if (!cIt->isEmpty() && cIt != (*cgi).end() )
                cDownloadPath = cIt->getValue();
            else
            {
                this->lastPage (in, out);
                LOG4CPLUS_ERROR (fLogger, RED << "The path to download the Image to has not been specified!" << RESET);
                return;
            }
        }

        if (cUpload)
        {
            form_iterator cIt = cgi.getElement ("imagename");

            if (!cIt->isEmpty() && cIt != (*cgi).end() )
                cImageName = cIt->getValue();
            else
            {
                this->lastPage (in, out);
                LOG4CPLUS_ERROR (fLogger, RED << "The Image name has not been specified!" << RESET);
                return;
            }

            const_file_iterator cFile = cgi.getFile ("Image");

            if (cFile != cgi.getFiles().end() )
            {
                std::ofstream cTmp ("/tmp/myImage.bin");

                cFile->writeToStream (cTmp);
                cImagePath = "/tmp/myImage.bin";
            }
            else
            {
                this->lastPage (in, out);
                LOG4CPLUS_ERROR (fLogger, RED << "No file Uploaded!" << RESET);
                return;
            }
        }

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (fLogger, RED << e.what() << RESET );
    }

    try
    {
        std::ostringstream cLogStream;
        Ph2_System::SystemController fSystemController;

        this->prepare_System_initialise (cLogStream);

        fSystemController.InitializeHw (this->fHwXMLString, cLogStream, false);
        BeBoard* pBoard = fSystemController.fBoardVector.at (0);

        if (cLoad)
        {
            fSystemController.fBeBoardInterface->JumpToFpgaConfig (pBoard, cImage);
            exit (0);
        }
        else if (cDelete)
            fSystemController.fBeBoardInterface->DeleteFpgaConfig (pBoard, cImage);
        else if (cDownload) fSystemController.fBeBoardInterface->DownloadFpgaConfig (pBoard, cImage, cDownloadPath );
        else if (cUpload)
        {
            fSystemController.fBeBoardInterface->FlashProm (pBoard, cImageName, cImagePath.c_str() );
            uint32_t progress;
            bool cDone = false;

            while (!cDone)
            {
                progress = fSystemController.fBeBoardInterface->getConfiguringFpga (pBoard)->getProgressValue();

                if (progress == 100)
                {
                    cDone = true;
                    LOG4CPLUS_INFO (fLogger, BOLDBLUE << "100\% Done" << RESET);
                }
                else
                {
                    LOG4CPLUS_INFO (fLogger, BLUE << "\% " << fSystemController.fBeBoardInterface->getConfiguringFpga (pBoard)->getProgressString() << " done\r" << RESET);
                    sleep (1);
                }
            }
        }

        fSystemController.Destroy();
        //delete fSystemController;
        //fSystemController = nullptr;

        //since we are cleaning up behind us, we need to clear the HwXMLString
        this->fHwXMLString.clear();
    }
    catch (const std::exception& e)
    {
        XCEPT_RAISE (xgi::exception::Exception, e.what() );
    }

    this->lastPage (in, out);
}

void SupervisorGUI::processDSForm (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    try
    {
        Cgicc cgi (in);

        *fSendData = cgi.queryCheckbox ("send_data");

        for (auto cIt : *cgi)
        {

            if (cIt.getValue() != "")
            {

                if (cIt.getName() == "sourcehost") *fSourceHost = cIt.getValue().c_str();
                else if (cIt.getName() == "sourceport") *fSourcePort = stringtoint (cIt.getValue().c_str() );
                else if (cIt.getName() == "sinkhost") *fSinkHost = cIt.getValue().c_str();
                else if (cIt.getName() == "sinkport") *fSinkPort = stringtoint (cIt.getValue().c_str() );
            }
        }
    }
    catch (const std::exception& e)
    {
        XCEPT_RAISE (xgi::exception::Exception, e.what() );
    }

    this->lastPage (in, out);
}

void SupervisorGUI::processPlaybackForm (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    try
    {

        Cgicc cgi (in);

        *fPlaybackMode = cgi.queryCheckbox ("playbackmode");

        for (auto cIt : *cgi)
        {
            if (cIt.getValue() != "")
            {

                if (cIt.getName() == "playbackfile")
                {
                    *fPlaybackFile = cIt.getValue().c_str();
                    LOG4CPLUS_INFO (fLogger, GREEN << "Changed playback file to: " << BLUE << fPlaybackFile->toString() << RESET);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        XCEPT_RAISE (xgi::exception::Exception, e.what() );
    }

    this->lastPage (in, out);
}
