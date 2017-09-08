#include "DTCSupervisor/SupervisorGUI.h"
#include <fstream>

using namespace Ph2TkDAQ;

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
    fAutoRefresh (false),
    fSystemController (nullptr)
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
    //xgi::bind (this, &SupervisorGUI::Default, "Default");
    xgi::bind (this, &SupervisorGUI::MainPage, "MainPage");
    xgi::bind (this, &SupervisorGUI::ConfigPage, "ConfigPage");
    xgi::bind (this, &SupervisorGUI::CalibrationPage, "CalibrationPage");
    xgi::bind (this, &SupervisorGUI::FirmwarePage, "FirmwarePage");

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
    *out << cgicc::h3 ("DTC Supervisor Main Page") << std::endl;
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
    *out << cgicc::h3 ("DTC Supervisor HwDescription Page") << std::endl;

    char cState = fFSM->getCurrentState();

    *out << cgicc::table() << std::endl;
    this->displayLoadForm (in, out);
    this->displayDumpForm (in, out);
    *out << cgicc::table() << std::endl;

    // Display the HwDescription HTML form
    std::string url = fURN + "handleHWFormData";

    *out << cgicc::form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data").set ("onload", "DisplayFieldsOnload();") << std::endl;

    if (cState != 'E')
    {
        *out << cgicc::input().set ("type", "submit").set ("title", "submit the entered values").set ("value", "Submit") << std::endl;
        *out << cgicc::input().set ("type", "reset").set ("title", "reset the form").set ("value", "Reset") << std::endl;
    }
    else
    {
        *out << cgicc::input().set ("type", "submit").set ("title", "submit the entered values").set ("value", "Submit").set ("disabled", "true") << std::endl;
        *out << cgicc::input().set ("type", "reset").set ("title", "reset the form").set ("value", "Reset").set ("disabled", "true") << std::endl;
    }


    *out << fHWFormString << std::endl;
    *out << cgicc::form() << std::endl;
    this->createHtmlFooter (in, out);

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::FirmwarePage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;
    //define view and create header
    fCurrentPageView = Tab::FIRMWARE;
    this->createHtmlHeader (in, out, fCurrentPageView);
    *out << cgicc::h3 ("DTC Supervisor Firmware Page") << std::endl;

    char cState = fFSM->getCurrentState();

    this->displayLoadForm (in, out);

    *out << cgicc::div().set ("style", "display:inline-block") << std::endl;

    if (cState != 'I')
    {
        *out << cgicc::p (cgicc::span ("Firmware operations can only be done in Initial State!!").set ("style", "color:red; font-size: 20pt") ).set ("style", "margin-top: 20px") << std::endl;
        *out << cgicc::p ().add (cgicc::span ("Current State: ").set ("style", "color:blue; font-size: 20pt") ).add (cgicc::span (fFSM->getStateName (cState) ).set ("style", "color:red; font-size: 20pt") ) << std::endl;
    }

    else
    {
        //display the form
        *out << cgicc::legend ("Fw Settings") << cgicc::fieldset() << std::endl;
        //the button / form for loading the stuff
        *out << cgicc::form().set ("method", "POST").set ("action", fURN + "loadImages").set ("enctype", "multipart/form-data").add (cgicc::table().add (cgicc::tr().add (cgicc::td (cgicc::input ().set ("type", "submit").set ("title", "load list of FW images from FC7").set ("name", "listImages").set ("value", "List Images").set ("style", "margin-bottom: 20px") ) ).add (cgicc::td (cgicc::input ().set ("type", "submit").set ("title", "reboot the FC7").set ("name", "rebootBoard").set ("value", "Reboot Board").set ("style", "margin-bottom: 20px") ) ) ) ) << std::endl;
        //the select node to select the image
        *out << cgicc::form ().set ("method", "POST").set ("action", fURN + "handleImages").set ("enctype", "multipart/form-data") << std::endl;
        *out << "<table>" << std::endl;
        *out << cgicc::tr() << std::endl;
        *out << cgicc::td() << cgicc::label ("Image List") << cgicc::select ().set ("name", "FW images").set ("style", "margin-bottom: 20px") << std::endl;

        if (fImageVector.size() )
        {
            for (auto cImage : fImageVector)
                *out << cgicc::option (cImage) << std::endl;
        }

        *out << cgicc::select() << cgicc::td() << std::endl;
        *out << cgicc::tr() << std::endl;

        *out << cgicc::tr() << std::endl;
        //now put a couple of inputs(submit type there with the various actions)
        *out << cgicc::td (cgicc::input ().set ("type", "submit").set ("name", "loadImage").set ("title", "configure FPGA with selected FW image").set ("value", "Load Image").set ("style", "margin-bottom: 20px") ) << std::endl;
        *out << cgicc::td (cgicc::input ().set ("type", "submit").set ("name", "deleteImage").set ("title", "delete selected FW image").set ("value", "Delete Image").set ("style", "margin-bottom: 20px") ) << std::endl;
        *out << cgicc::tr() << std::endl;

        *out << cgicc::tr().add (cgicc::td().add (cgicc::label ("Path to Download:") ).add (cgicc::input ().set ("type", "text").set ("name", "pathdownload").set ("size", "70").set ("value", expandEnvironmentVariables (HOME) + "/myImage.bin" ).set ("title", "download selected FW image").set ("style", "margin-bottom: 20px") ) ) << std::endl;
        *out << cgicc::tr().add (cgicc::td (cgicc::input ().set ("type", "submit").set ("name", "downloadImage").set ("title", "download selected FW image").set ("value", "Download").set ("style", "margin-bottom: 20px") ) ) << std::endl;

        *out << cgicc::tr().add (cgicc::td().add (cgicc::label ("Image to upload:") ).add (cgicc::input ().set ("type", "file").set ("name", "Image").set ("size", "70").set ("style", "margin-bottom: 20px") ) ).add (cgicc::td().add (cgicc::label ("Image Name:") ).add (cgicc::input ().set ("type", "text").set ("name", "imagename").set ("size", "30").set ("value", "myImage.bin" ).set ("title", "name for uploaded FW image on SD Card").set ("style", "margin-bottom: 20px") ) ) << std::endl;
        *out << cgicc::tr().add (cgicc::td (cgicc::input ().set ("type", "submit").set ("name", "uploadImage").set ("title", "upload selected FW image").set ("value", "Upload").set ("style", "margin-bottom: 20px") ) ) << std::endl;

        *out << "</table>" << std::endl;
        *out << cgicc::form() << std::endl;

        *out << cgicc::fieldset() << std::endl;
    }

    *out << cgicc::div() << std::endl;


    //first, we are messing with Firmware, so before we do anything, we should destroy the running configuration
    //or maybe this should happen the very second I am actually taking action
    //if (cState != 'I')
    //fFSM->fireEvent ("Destroy");

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
    *out << cgicc::style (parseExternalResource (expandEnvironmentVariables (CSSSTYLESHEET), cLogStream) ) << std::endl;
    // some javascript
    std::string JSfile = expandEnvironmentVariables (HOME);

    bool cAutoRefresh = false;
    int cRefreshDelay = 3;

    if (cState == 'E' || fAutoRefresh)
        cAutoRefresh = true;

    std::ostringstream cAutoRefreshString;
    cAutoRefreshString << cRefreshDelay << "; " << fURN;

    std::ostringstream cTabBarString;

    // switch to show the current tab
    switch (pTab)
    {
        case Tab::MAIN:
            cTabBarString << cgicc::a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button active") << cgicc::a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button") << cgicc::a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button") << cgicc::a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button") ;
            JSfile += "/html/formfields.js";
            cAutoRefreshString << "MainPage";
            break;

        case Tab::CONFIG:
            cTabBarString << cgicc::a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button") << cgicc::a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button active") << cgicc::a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button") << cgicc::a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button");
            JSfile += "/html/HWForm.js";
            cAutoRefreshString << "ConfigPage";
            break;

        case Tab::CALIBRATION:
            cTabBarString << cgicc::a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button") << cgicc::a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button") << cgicc::a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button active") << cgicc::a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button");
            JSfile += "/html/empty.js";
            cAutoRefreshString << "CalibrationPage";
            break;

        case Tab::FIRMWARE:
            cTabBarString << cgicc::a ("Main Page").set ("href", fURN + "MainPage").set ("class", "button") << cgicc::a ("Config Page").set ("href", fURN + "ConfigPage").set ("class", "button") << cgicc::a ("Calibration Page").set ("href", fURN + "CalibrationPage").set ("class", "button") << cgicc::a ("Firmware Page").set ("href", fURN + "FirmwarePage").set ("class", "button active");
            JSfile += "/html/empty.js";
            cAutoRefreshString << "FirmwarePage";
            break;
    }

    //if(!cAutoRefresh)

    if (cAutoRefresh)
    {
        *out << cgicc::meta().set ("HTTP-EQUIV", "Refresh").set ("CONTENT", cAutoRefreshString.str() ) << std::endl;
        cTabBarString << cgicc::a ("AutoRefresh Off").set ("href", fURN + "toggleAutoRefresh" ).set ("class", "refresh_button_active") << std::endl;
    }
    else
        cTabBarString << cgicc::a ("AutoRefresh On").set ("href", fURN + "toggleAutoRefresh" ).set ("class", "refresh_button") << std::endl;

    //*out << " <meta HTTP-EQUIV=\"Refresh\" CONTENT=\"" << cRefreshDelay << "; " << fURN << "MainPage\"/>" << std::endl;

    *out << cgicc::script (parseExternalResource (JSfile, cLogStream) ).set ("type", "text/javascript") << std::endl;
    *out << cgicc::div (cgicc::h1 (cgicc::a ("DTC Supervisor").set ("href", fURN + "MainPage") ) ).set ("class", "title") << std::endl;
    *out << cgicc::div (cTabBarString.str() ).set ("class", "tab") << std::endl;
    *out << "<div class=\"main\">" << std::endl;
    this->showStateMachineStatus (out);
    *out << "<div class=\"content\">" << std::endl;

    if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::createHtmlFooter (xgi::Input* in, xgi::Output* out)
{
    //close main and content divs
    *out << "</div>" <<  std::endl << "</div>" << std::endl;
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

        *out << "<div class=\"sidenav\">" << std::endl;
        *out << cgicc::p (cCurrentState.str() ).set ("class", "state") << std::endl;
        *out << cgicc::p (std::string (1, cState) ).set ("style", "display:none").set ("id", "state") << std::endl;

        //Event counter
        *out << cgicc::div().set ("class", "eventcount") << std::endl;
        *out << cgicc::table() << std::endl;
        *out << cgicc::tr().add (cgicc::td ("Requested Events: ") ).add (cgicc::td (fNEvents->toString() ) ) << std::endl;
        *out << cgicc::tr().add (cgicc::td ("Event Count: ") ).add (cgicc::td (std::to_string (*fEventCounter ) ) ) << std::endl;
        *out << cgicc::table() << std::endl;
        *out << cgicc::div()  << std::endl;

        // display FSM
        // I could loop all possible transitions and check if they are allowed but that does not put the commands sequentially
        // https://gitlab.cern.ch/cms_tk_ph2/BoardSupervisor/blob/master/src/common/BoardSupervisor.cc

        //the action is the fsmTransitionHandler
        std::string url = fURN + "fsmTransition";
        *out << cgicc::form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data") << std::endl;
        //first the refresh button
        *out << cgicc::input().set ("type", "submit").set ("name", "transition").set ("value", "Refresh" ).set ("class", "refresh") << std::endl;
        this->createStateTransitionButton ("Initialise", out);
        this->createStateTransitionButton ("Configure", out);
        this->createStateTransitionButton ("Enable", out);
        this->createStateTransitionButton ("Stop", out);
        this->createStateTransitionButton ("Pause", out);
        this->createStateTransitionButton ("Resume", out);
        this->createStateTransitionButton ("Halt", out);
        this->createStateTransitionButton ("Destroy", out);
        *out << cgicc::form() << std::endl;

        std::string cFilename = fHWDescriptionFile->toString().substr (fHWDescriptionFile->toString().find_last_of ("/") + 1);
        *out << cgicc::div ("Current HW File: " + cFilename).set ("class", "current_file") << std::endl;
        *out << "</div>" << std::endl;
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
        cgicc::Cgicc cgi (in);
        std::string cTransition = cgi["transition"]->getValue();

        //here handle whatever action is necessary on the gui before handing over to the main application
        if (cTransition == "Initialise" )
        {
            if ( fHWFormString.empty() )
                this->loadHWFile();

            //now convert the HW Description HTMLString to an xml string for Initialize of Ph2ACF
            std::string cTmpFormString = cleanup_before_XSLT (fHWFormString);
            fHwXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (XMLSTYLESHEET), cLogStream, false);
            //now do the same for the Settings
            cTmpFormString = cleanup_before_XSLT_Settings (fSettingsFormString);
            fSettingsXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (SETTINGSSTYLESHEETINVERSE), cLogStream, false);
        }

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


void SupervisorGUI::displayLoadForm (xgi::Input* in, xgi::Output* out)
{
    //get FSM state
    char cState = fFSM->getCurrentState();

    std::string url = fURN + "reloadHWFile";

    //*out << cgicc::div().set ("padding", "10px") << std::endl;

    //only allow changing the HW Description File in state initial
    *out << cgicc::form().set ("padding", "10px").set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data").set ("autocomplete", "on") << std::endl;

    if (cState == 'I')
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Hw Description File Path:").set ("for", "HwDescriptionFile") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "HwDescriptionFile").set ("id", "HwDescriptionFile").set ("size", "70").set ("value", fHWDescriptionFile->toString() ) ) ).add (cgicc::td (cgicc::input().set ("type", "submit").set ("id", "hwForm_load_submit").set ("title", "change the Hw Description File").set ("value", "Reload") ) ) << std::endl;
    else
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Hw Description File Path:").set ("for", "HwDescriptionFile") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "HwDescriptionFile").set ("id", "HwDescriptionFile").set ("size", "70").set ("value", fHWDescriptionFile->toString() ).set ("disabled", "disabled")  ) ).add (cgicc::td (cgicc::input().set ("type", "submit").set ("id", "hwForm_load_submit").set ("title", "change the Hw Description File").set ("value", "Reload").set ("disabled", "disabled") ) ) << std::endl;

    *out << cgicc::form() << std::endl;
}

void SupervisorGUI::reloadHWFile (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    // stream for logger
    std::ostringstream cLogStream;
    std::string cHWDescriptionFile;

    try
    {
        //parse the form input
        cgicc::Cgicc cgi (in);
        cgicc::form_iterator cIt = cgi.getElement ("HwDescriptionFile");

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
        fHWFormString = XMLUtils::transformXmlDocument (fHWDescriptionFile->toString(), expandEnvironmentVariables (XSLSTYLESHEET), cLogStream);
        cleanup_after_XSLT (fHWFormString);
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
    *out << cgicc::form().set ("style", "padding-top:10px").set ("style", "padding-bottom:10px").set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data") << std::endl;
    *out << cgicc::tr().add (cgicc::td (cgicc::label ("Path for Hw File dump: ").set ("for", "fileDumpPath") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "fileDumpPath").set ("title", "file path to dump HW Description File").set ("value", expandEnvironmentVariables (HOME) ).set ("size", "70") ) ).add (cgicc::td (cgicc::input().set ("type", "submit").set ("title", "dump HWDescription form to XML File").set ("value", "Dump") ) ) << std::endl;
    *out << cgicc::form() << std::endl;
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
            cgicc::Cgicc cgi (in);
            cgicc::form_iterator cIt = cgi.getElement ("fileDumpPath");

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

    *out << "<div style=\"display: inline-block; widht=100%\">" << std::endl;

    *out << cgicc::form().set ("id", "acfform").set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data").set ("onclick", "displayACFfields();") << std::endl;

    //left form part
    *out << cgicc::div().set ("class", "acf_left") << std::endl;

    //Procedure settings
    if (cState == 'E')
        *out << cgicc::fieldset().set ("id", "procedurelist").set ("style", "margin-top:20px").set ("onblur", "submitACFForm();").set ("disabled", "disabled") << cgicc::legend ("Ph2_ACF Main Settings") << std::endl;
    else
        *out << cgicc::fieldset().set ("id", "procedurelist").set ("style", "margin-top:20px").set ("onblur", "submitACFForm();") << cgicc::legend ("Ph2_ACF Main Settings") << std::endl;

    this->createProcedureInput (out);
    *out << cgicc::fieldset() << std::endl;
    //end of procedure settings

    //Main Settings parsed from file!
    if (cState == 'E')
        *out << cgicc::fieldset().set ("style", "margin-top:10px; display:none").set ("id", "settings_fieldset") << cgicc::legend ("Application Settings").set ("id", "settings_fieldset_legend").set ("style", "display:none").set ("disabled", "disabled") << std::endl;
    else
        *out << cgicc::fieldset().set ("style", "margin-top:10px; display:none").set ("id", "settings_fieldset") << cgicc::legend ("Application Settings").set ("id", "settings_fieldset_legend").set ("style", "display:none") << std::endl;

    *out << fSettingsFormString << std::endl;
    *out << cgicc::input().set ("type", "button").set ("value", "Add Setting").set ("onClick", "addSetting('settings_table');") << std::endl;
    *out << cgicc::fieldset() << std::endl;
    //end of Main Settings
    *out << cgicc::div() << std::endl;

    //right form part
    *out << cgicc::div().set ("class", "acf_right") << std::endl;

    //main run settings
    *out << cgicc::fieldset().set ("style", "margin-top:20px") << cgicc::legend ("Main Run Settings") << std::endl;
    *out << cgicc::table() << std::endl;
    *out << cgicc::tr() << std::endl;

    if (cState != 'I')
    {

        if (*fRAWFile)
            *out << cgicc::td().add (cgicc::label ( "Write RAW File" ) ).add (cgicc::input().set ("type", "checkbox").set ("name", "raw_file").set ("value", "on").set ("checked", "checked").set ("id", "fileOptions_raw").set ("disabled", "disabled") ) <<  std::endl;
        else
            *out << cgicc::td().add (cgicc::label ( "Write RAW File" ) ).add (cgicc::input().set ("type", "checkbox").set ("name", "raw_file").set ("value", "on").set ("id", "fileOptions_raw").set ("disabled", "disabled") ) <<  std::endl;

        if (*fDAQFile)
            *out << cgicc::td().add (cgicc::label ("Write DAQ File" ) ).add (cgicc::input().set ("type", "checkbox").set ("name", "daq_file").set ("value", "on").set ("checked", "checked").set ("id", "fileOptions_daq").set ("disabled", "disabled") ) << std::endl;
        else
            *out << cgicc::td().add (cgicc::label ("Write DAQ File" ) ).add (cgicc::input().set ("type", "checkbox").set ("name", "daq_file").set ("value", "on").set ("id", "fileOptions_daq").set ("disabled", "disabled") ) << std::endl;
    }
    else
    {
        if (*fRAWFile)
            *out << cgicc::td().add (cgicc::label ("Write RAW File") ).add (cgicc::input().set ("type", "checkbox").set ("name", "raw_file").set ("value", "on").set ("checked", "checked").set ("id", "fileOptions_raw") ) << std::endl;
        else
            *out << cgicc::td().add (cgicc::label ("Write RAW File") ).add (cgicc::input().set ("type", "checkbox").set ("name", "raw_file").set ("value", "on").set ("id", "fileOptions_raw") ) << std::endl;

        if (*fDAQFile)
            *out << cgicc::td().add (cgicc::label ("Write DAQ File") ).add (cgicc::input().set ("type", "checkbox").set ("name", "daq_file").set ("value", "on").set ("checked", "checked").set ("id", "fileOptions_daq") )  << std::endl;
        else
            *out << cgicc::td().add (cgicc::label ("Write DAQ File") ).add (cgicc::input().set ("type", "checkbox").set ("name", "daq_file").set ("value", "on").set ("id", "fileOptions_daq") )  << std::endl;
    }

    *out << cgicc::tr() << std::endl;

    if (cState != 'I')
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Runnumber: ") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "runnumber").set ("size", "20").set ("value", fRunNumber->toString() ).set ("id", "runnumber").set ("disabled", "disabled") ) ) << std::endl;
    else
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Runnumber: ") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "runnumber").set ("size", "20").set ("value", fRunNumber->toString() ).set ("id", "runnumber") ) ) << std::endl;


    if (cState == 'E')
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Number of Events: ") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "nevents").set ("size", "20").set ("placeholder", "number of events to take").set ("value", fNEvents->toString() ).set ("id", "nevents").set ("disabled", "disabled") ) ) << std::endl;
    else
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Number of Events: ") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "nevents").set ("size", "20").set ("placeholder", "number of events to take").set ("value", fNEvents->toString() ).set ("id", "nevents") ) ) << std::endl;


    if (cState != 'I')
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Data Directory: ") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "data_directory").set ("size", "60").set ("value", expandEnvironmentVariables (fDataDirectory->toString() ) ).set ("id", "datadir").set ("disabled", "disabled") ) ) << std::endl;
    else
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Data Directory: ") ) ).add (cgicc::td (cgicc::input().set ("type", "text").set ("name", "data_directory").set ("size", "60").set ("value", expandEnvironmentVariables (fDataDirectory->toString() ) ).set ("id", "datadir") ) ) << std::endl;


    if (cState == 'E')
        *out << cgicc::tr().add ( cgicc::td (cgicc::label ("Result Directory: ").set ("style", "display:none").set ("id", "result_directory_label") ) ).add (cgicc::input().set ("type", "text").set ("name", "result_directory").set ("id", "result_directory").set ("size", "60").set ("value", expandEnvironmentVariables (fResultDirectory->toString() ) ).set ("style", "display:none").set ("id", "resultdir").set ("disabled", "disabled") )  << std::endl;
    else
        *out << cgicc::tr().add (cgicc::td (cgicc::label ("Result Directory: ").set ("style", "display:none").set ("id", "result_directory_label") ) ).add (cgicc::input().set ("type", "text").set ("name", "result_directory").set ("id", "result_directory").set ("size", "60").set ("value", expandEnvironmentVariables (fResultDirectory->toString() ) ).set ("style", "display:none").set ("id", "resultdir") ) << std::endl;

    *out << cgicc::table() << std::endl;
    *out << cgicc::fieldset() << std::endl;
    //end of main run settings

    // commissioning settings
    if (cState == 'E')
        *out << cgicc::fieldset().set ("id", "commission_fieldset").set ("style", "margin:10px").set ("disabled", "disabled") << cgicc::legend ("Commissioning Settings").set ("id", "commission_legend") << std::endl;
    else
        *out << cgicc::fieldset().set ("id", "commission_fieldset").set ("style", "margin:10px") << cgicc::legend ("Commissioning Settings").set ("id", "commission_legend") << std::endl;

    //Commissioning Settings
    *out << cgicc::table().set ("style", "display:none").set ("id", "commission_table") << std::endl;
    *out << cgicc::tr() << std::endl;

    if (fLatency)
        *out << cgicc::td().add (cgicc::label ("Latency Scan") ).add (cgicc::input().set ("type", "checkbox").set ("name", "latency").set ("value", "on").set ("checked", "checked") )  << std::endl;
    else
        *out << cgicc::td().add (cgicc::label ("Latency Scan") ).add (cgicc::input().set ("type", "checkbox").set ("name", "latency").set ("value", "on") )  << std::endl;

    if (fStubLatency)
        *out << cgicc::td().add (cgicc::label ("Stub Latency Scan") ).add (cgicc::input().set ("type", "checkbox").set ("name", "stublatency").set ("value", "on").set ("checked", "checked") ) << std::endl;
    else
        *out << cgicc::td().add (cgicc::label ("Stub Latency Scan") ).add (cgicc::input().set ("type", "checkbox").set ("name", "stublatency").set ("value", "on") ) << std::endl;

    *out << cgicc::tr() << std::endl;

    *out << cgicc::tr() << std::endl;
    *out << cgicc::td().add (cgicc::label ("Minimum Latency: ") ).add (cgicc::input().set ("type", "text").set ("name", "minimum_latency").set ("size", "5").set ("value", inttostring (fLatencyStartValue) ) ) << std::endl;
    *out << cgicc::td().add (cgicc::label ("Latency Range: ") ).add (cgicc::input().set ("type", "text").set ("name", "latency_range").set ("size", "5").set ("value", inttostring (fLatencyRange) ) ) << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::table() << std::endl;
    *out << cgicc::fieldset() << std::endl;
    //end of commissioning settings

    //*out << cgicc::fieldset() << std::endl;

    if (cState != 'E')
        *out << cgicc::input().set ("type", "submit").set ("name", "Submit").set ("id", "main_submit").set ("value", "Submit") << std::endl;
    else
        *out << cgicc::input().set ("type", "submit").set ("name", "Submit").set ("value", "Submit").set ("disabled", "disabled") << std::endl;

    *out << cgicc::div() << std::endl;
    //end of right div
    *out << cgicc::form() << std::endl;
    //end of overal form
    *out << "</div>" << std::endl;
    //end of main inline-block div

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
        cgicc::Cgicc cgi (in);

        for (auto cProc : fProcedures)
        {
            bool cOn = cgi.queryCheckbox (cProc);
            fProcedureMap[cProc] = cOn;
        }

        *fRAWFile = cgi.queryCheckbox ("raw_file");
        *fDAQFile = cgi.queryCheckbox ("daq_file");

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
    *out << cgicc::div (cgicc::legend ("Ph2_ACF Logs") ).set ("class", "legend") << std::endl;
    *out << cgicc::div (cgicc::p (fPh2_ACFLog) ).set ("class", "log").set ("id", "logwindow") << std::endl;
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
        cgicc::Cgicc cgi (in);

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
        cgicc::Cgicc cgi (in);

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

    char cState = fFSM->getCurrentState();

    try
    {
        if (cState != 'I')
        {
            fFSM->fireEvent ("Destroy", fApp);
            LOG4CPLUS_ERROR (fLogger, RED << "This should never happen!" << RESET);
            this->wait_state_changed (cState);
        }

        if (fSystemController != nullptr) delete fSystemController;

        fSystemController = new Ph2_System::SystemController();

        if (this->fHwXMLString.empty() )
        {
            // we haven't created the xml string yet, this normally happens on configure
            if ( fHWFormString.empty() )
                this->loadHWFile();

            //now convert the HW Description HTMLString to an xml string for Initialize of Ph2ACF
            std::string cTmpFormString = cleanup_before_XSLT (fHWFormString);
            this->fHwXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (XMLSTYLESHEET), cLogStream, false);
        }

        //if we don't find an environment variable in the path to the address tabel, or the path does not point to Ph2_ACF root we prepend the Ph2ACF ROOT
        if (this->fHwXMLString.find ("file:://${") == std::string::npos || this->fHwXMLString.find ("file://" + expandEnvironmentVariables ("${PH2ACF_ROOT}") ) == std::string::npos)
        {
            std::string cCorrectPath = "file://" + expandEnvironmentVariables ("${PH2ACF_ROOT}") + "/";
            cleanupHTMLString (this->fHwXMLString, "file://", cCorrectPath);
        }

        //the same goes for the CBC File Path
        if (this->fHwXMLString.find ("<CBC_Files path=\"${") == std::string::npos || this->fHwXMLString.find ("<CBC_Files path=\"" + expandEnvironmentVariables ("${PH2ACF_ROOT}") ) == std::string::npos)
        {
            std::string cCorrectPath = "<CBC_Files path=\"" + expandEnvironmentVariables ("${PH2ACF_ROOT}");
            cleanupHTMLString (this->fHwXMLString, "<CBC_Files path=\".", cCorrectPath);
        }

        fSystemController->InitializeHw (this->fHwXMLString, cLogStream, false);
        BeBoard* pBoard = fSystemController->fBoardVector.at (0);

        if (cList)
            fImageVector = fSystemController->fBeBoardInterface->getFpgaConfigList (pBoard);
        else if (cReboot)
            LOG4CPLUS_INFO (fLogger, GREEN << "Rebooting Board - this will cause a segfault and needs a restart of the application!" << RESET);

        fSystemController->fBeBoardInterface->RebootBoard (pBoard);

        fSystemController->Destroy();
        delete fSystemController;
        fSystemController = nullptr;

        //since we are cleaning up behind us, we need to clear the HwXMLString
        this->fHwXMLString.clear();
    }
    catch (const std::exception& e)
    {
        XCEPT_RAISE (xgi::exception::Exception, e.what() );
    }

    this->lastPage (in, out);
}

void SupervisorGUI::handleImages (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;

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
        cgicc::Cgicc cgi (in);

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
            cgicc::form_iterator cIt = cgi.getElement ("FW Images");

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
            cgicc::form_iterator cIt = cgi.getElement ("pathdownload");

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
            cgicc::form_iterator cIt = cgi.getElement ("imagename");

            if (!cIt->isEmpty() && cIt != (*cgi).end() )
                cImageName = cIt->getValue();
            else
            {
                this->lastPage (in, out);
                LOG4CPLUS_ERROR (fLogger, RED << "The Image name has not been specified!" << RESET);
                return;
            }

            cgicc::const_file_iterator cFile = cgi.getFile ("Image");

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

        std::cout << cUpload << " " << cDownload << " " << cLoad << " " << cDelete << " " << cImage << " " << std::endl;
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (fLogger, RED << e.what() << RESET );
    }

    char cState = fFSM->getCurrentState();

    try
    {
        if (cState != 'I')
        {
            fFSM->fireEvent ("Destroy", fApp);
            LOG4CPLUS_ERROR (fLogger, RED << "This should never happen!" << RESET);
            this->wait_state_changed (cState);
        }

        if (fSystemController != nullptr) delete fSystemController;

        fSystemController = new Ph2_System::SystemController();

        if (this->fHwXMLString.empty() )
        {
            // we haven't created the xml string yet, this normally happens on configure
            if ( fHWFormString.empty() )
                this->loadHWFile();

            //now convert the HW Description HTMLString to an xml string for Initialize of Ph2ACF
            std::string cTmpFormString = cleanup_before_XSLT (fHWFormString);
            this->fHwXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (XMLSTYLESHEET), cLogStream, false);
        }

        //if we don't find an environment variable in the path to the address tabel, or the path does not point to Ph2_ACF root we prepend the Ph2ACF ROOT
        if (this->fHwXMLString.find ("file:://${") == std::string::npos || this->fHwXMLString.find ("file://" + expandEnvironmentVariables ("${PH2ACF_ROOT}") ) == std::string::npos)
        {
            std::string cCorrectPath = "file://" + expandEnvironmentVariables ("${PH2ACF_ROOT}") + "/";
            cleanupHTMLString (this->fHwXMLString, "file://", cCorrectPath);
        }

        //the same goes for the CBC File Path
        if (this->fHwXMLString.find ("<CBC_Files path=\"${") == std::string::npos || this->fHwXMLString.find ("<CBC_Files path=\"" + expandEnvironmentVariables ("${PH2ACF_ROOT}") ) == std::string::npos)
        {
            std::string cCorrectPath = "<CBC_Files path=\"" + expandEnvironmentVariables ("${PH2ACF_ROOT}");
            cleanupHTMLString (this->fHwXMLString, "<CBC_Files path=\".", cCorrectPath);
        }

        fSystemController->InitializeHw (this->fHwXMLString, cLogStream, false);
        BeBoard* pBoard = fSystemController->fBoardVector.at (0);
        std::cout << "I get here!" << std::endl;

        if (cLoad)
        {
            fSystemController->fBeBoardInterface->JumpToFpgaConfig (pBoard, cImage);
            exit (0);
        }
        else if (cDelete) fSystemController->fBeBoardInterface->DeleteFpgaConfig (pBoard, cImage);
        else if (cDownload) fSystemController->fBeBoardInterface->DownloadFpgaConfig (pBoard, cImage, cDownloadPath );
        else if (cUpload)
        {
            fSystemController->fBeBoardInterface->FlashProm (pBoard, cImageName, cImagePath.c_str() );
            uint32_t progress;
            bool cDone = false;

            while (!cDone)
            {
                progress = fSystemController->fBeBoardInterface->getConfiguringFpga (pBoard)->getProgressValue();

                if (progress == 100)
                {
                    cDone = true;
                    LOG4CPLUS_INFO (fLogger, BOLDBLUE << "100\% Done" << RESET);
                }
                else
                {
                    LOG4CPLUS_INFO (fLogger, BLUE << "\% " << fSystemController->fBeBoardInterface->getConfiguringFpga (pBoard)->getProgressString() << " done\r" << RESET);
                    sleep (1);
                }
            }
        }

        fSystemController->Destroy();
        delete fSystemController;
        fSystemController = nullptr;

        //since we are cleaning up behind us, we need to clear the HwXMLString
        this->fHwXMLString.clear();
    }
    catch (const std::exception& e)
    {
        XCEPT_RAISE (xgi::exception::Exception, e.what() );
    }

    this->lastPage (in, out);
}
