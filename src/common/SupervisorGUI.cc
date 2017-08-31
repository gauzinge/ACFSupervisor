#include "DTCSupervisor/SupervisorGUI.h"
#include <fstream>

using namespace Ph2TkDAQ;

SupervisorGUI::SupervisorGUI (xdaq::WebApplication* pApp, DTCStateMachine* pStateMachine) :
    fApp (pApp),
    fManager (pApp),
    fFSM (pStateMachine),
    fHWDescriptionFile (nullptr),
    //fXLSStylesheet (nullptr),
    fHWFormString (""),
    fHwXMLString ("")
{
    fLogger = pApp->getApplicationLogger();
    fURN =  pApp->getApplicationDescriptor()->getContextDescriptor()->getURL() + "/" + pApp->getApplicationDescriptor()->getURN() + "/";

    //bind xgi and xoap commands to methods
    //methods for tab navigation
    //xgi::bind (this, &SupervisorGUI::Default, "Default");
    xgi::bind (this, &SupervisorGUI::MainPage, "MainPage");
    xgi::bind (this, &SupervisorGUI::ConfigPage, "ConfigPage");
    xgi::bind (this, &SupervisorGUI::CalibrationPage, "CalibrationPage");
    xgi::bind (this, &SupervisorGUI::DAQPage, "DAQPage");

    //helper methods for buttons etc
    xgi::bind (this, &SupervisorGUI::reloadHWFile, "reloadHWFile");
    xgi::bind (this, &SupervisorGUI::dumpHWDescription, "dumpHWDescription");
    xgi::bind (this, &SupervisorGUI::fsmTransition, "fsmTransition");
    xgi::bind (this, &SupervisorGUI::handleHWFormData, "handleHWFormData");
    xgi::bind (this, &SupervisorGUI::lastPage, "lastPage");

    fCurrentPageView = Tab::MAIN;
}

void SupervisorGUI::MainPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::string url = fURN + "reloadHWFile";

    // stream for logger
    std::ostringstream cLogStream;

    //define view and create header
    fCurrentPageView = Tab::MAIN;
    this->createHtmlHeader (in, out, fCurrentPageView);

    // generate the page content
    *out << cgicc::h3 ("DTC Supervisor Main Page") << std::endl;
    this->displayLoadForm (in, out);
    this->displayPh2_ACFForm (in, out);

    //LOG4CPLUS_INFO (fLogger, cLogStream.str() );
    this->createHtmlFooter (in, out);
}

void SupervisorGUI::ConfigPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;
    //define view and create header
    fCurrentPageView = Tab::CONFIG;
    this->createHtmlHeader (in, out, fCurrentPageView);

    char cState = fFSM->getCurrentState();

    this->displayLoadForm (in, out);
    this->displayDumpForm (in, out);

    // Display the HwDescription HTML form
    // only allow changes in Initial and Configured
    std::string url = fURN + "handleHWFormData";

    std::string JSfile = expandEnvironmentVariables (HOME);
    JSfile += "/html/HWForm.js";

    *out << cgicc::script().set ("type", "text/javascript") << std::endl;
    *out << parseExternalResource (JSfile, cLogStream) << std::endl;
    *out << cgicc::script() << std::endl;
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
    LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::createHtmlHeader (xgi::Input* in, xgi::Output* out, Tab pTab)
{
    // Create the Title, Tab bar
    std::ostringstream cLogStream;

    fManager.getHTMLHeader (in, out);
    //Style this thing
    *out << cgicc::style() << std::endl;
    *out << parseExternalResource (expandEnvironmentVariables (CSSSTYLESHEET), cLogStream) << std::endl;
    *out << cgicc::style() << std::endl;

    *out << cgicc::title ("DTC Supervisor")  << std::endl;

    std::ostringstream cTabBarString;
    std::string url = fURN;

    // switch to show the current tab
    switch (pTab)
    {
        case Tab::MAIN:
            cTabBarString << "<a href='" << url << "MainPage' class=\"button active\">MainPage</a>  <a href='" << url << "ConfigPage' class=\"button\">ConfigPage</a>  <a href='" << url << "CalibrationPage' class=\"button\">CalibrationPage</a>  <a href='" << url << "DAQPage' class=\"button\">DAQPage</a>" << std::endl;
            break;

        case Tab::CONFIG:
            cTabBarString << "<a href='" << url << "MainPage' class=\"button\">MainPage</a>  <a href='" << url << "ConfigPage' class=\"button active\">ConfigPage</a>  <a href='" << url << "CalibrationPage' class=\"button\">CalibrationPage</a>  <a href='" << url << "DAQPage' class=\"button\">DAQPage</a>" << std::endl;
            break;

        case Tab::CALIBRATION:
            cTabBarString << "<a href='" << url << "MainPage' class=\"button\">MainPage</a>  <a href='" << url << "ConfigPage' class=\"button\">ConfigPage</a>  <a href='" << url << "CalibrationPage' class=\"button active\">CalibrationPage</a>  <a href='" << url << "DAQPage' class=\"button\">DAQPage</a>" << std::endl;
            break;

        case Tab::DAQ:
            cTabBarString << "<a href='" << url << "MainPage' class=\"button\">MainPage</a>  <a href='" << url << "ConfigPage' class=\"button\">ConfigPage</a>  <a href='" << url << "CalibrationPage' class=\"button\">CalibrationPage</a>  <a href='" << url << "DAQPage' class=\"button active\">DAQPage</a>" << std::endl;
            break;
    }

    std::string link = url + "MainPage";
    *out << cgicc::div().set ("class", "title") << std::endl;
    *out << cgicc::h1 (cgicc::a ("DTC Supervisor").set ("href", url) ) << std::endl;
    *out << cgicc::div() << std::endl;
    *out << "<div class=\"tab\">" << std::endl;
    *out << cTabBarString.str() << std::endl;
    *out << "</div>" << std::endl;
    *out << "<div class=\"main\">" << std::endl;
    this->showStateMachineStatus (out);
    *out << "<div class=\"content\">" << std::endl;

    LOG4CPLUS_INFO (fLogger, cLogStream.str() );
}

void SupervisorGUI::createHtmlFooter (xgi::Input* in, xgi::Output* out)
{
    //close the main and content div
    *out << "</div class=\"content\"></div class=\"main\">" << std::endl;
    fManager.getHTMLFooter (in, out);

}

void SupervisorGUI::showStateMachineStatus (xgi::Output* out) throw (xgi::exception::Exception)
{
    // create the FSM Status bar showing the current state
    try
    {
        //the action is the fsmTransitionHandler
        std::string url = fURN + "fsmTransition";

        //display the sidenav with the FSM controls
        *out << "<div class=\"sidenav\">" << std::endl;
        *out << "<p class=\"state\">Current State: " << fFSM->getStateName (fFSM->getCurrentState() ) << "/" << fFSM->getCurrentState() << "</p>" << std::endl;
        *out << cgicc::br() << std::endl;

        // display FSM
        // I could loop all possible transitions and check if they are allowed but that does not put the commands sequentially
        // https://gitlab.cern.ch/cms_tk_ph2/BoardSupervisor/blob/master/src/common/BoardSupervisor.cc

        *out << cgicc::form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data") << std::endl;
        //first the refresh button
        *out << cgicc::input().set ("type", "submit").set ("name", "transition").set ("value", "Refresh" ).set ("class", "refresh") << std::endl;
        *out << cgicc::br() << std::endl;
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
        *out << cgicc::br() << std::endl;
        *out << cgicc::div().set ("class", "current_file") << "Current HW File: " << cFilename << cgicc::div()  << std::endl;
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

    try
    {
        //get the state transition tirggered by the button in the sidebar and propagate to the main DTC supervisor application
        //before take action as required by the state transition on any GUI elements
        cgicc::Cgicc cgi (in);
        std::string cTransition = cgi["transition"]->getValue();

        //here handle whatever action is necessary on the gui before handing over to the main application

        // if the transition is Initialise and the HW Form String is still empty, trigger the callback otherwise triggered by the load button
        if (cTransition == "Initialise" )
        {
            if ( fHWFormString.empty() )
                this->reloadHWFile (in, out);

            //now convert the HTMLString to an xml string for Initialize of Ph2ACF
            std::string cTmpFormString = cleanup_before_XSLT (fHWFormString);
            fHwXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (XMLSTYLESHEET), cLogStream, false);
            std::cout << fHwXMLString << std::endl;
        }


        if (cTransition == "Refresh")
            this->lastPage (in, out);
        else
            fFSM->fireEvent (cTransition, fApp);
    }
    catch (const std::exception& e)
    {
        XCEPT_RAISE (xgi::exception::Exception, e.what() );
    }

    LOG4CPLUS_INFO (fLogger, cLogStream.str() );
    this->lastPage (in, out);
}


void SupervisorGUI::displayLoadForm (xgi::Input* in, xgi::Output* out)
{
    //get FSM state
    char cState = fFSM->getCurrentState();

    std::string url = fURN + "reloadHWFile";

    *out << cgicc::div().set ("padding", "10px") << std::endl;

    //only allow changing the HW Description File in state initial
    *out << cgicc::form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data").set ("autocomplete", "on") << std::endl;
    *out << "<label for=\"HwDescriptionFile\">Hw Descritpion FilePath: </label>" << std::endl;

    if (cState == 'I' )
    {
        *out << cgicc::input().set ("type", "text").set ("name", "HwDescriptionFile").set ("id", "HwDescriptionFile").set ("size", "70").set ("value", fHWDescriptionFile->toString() ) << std::endl;
        *out << cgicc::input().set ("type", "submit").set ("title", "change the Hw Description File").set ("value", "Load") << std::endl;
    }
    else
    {
        *out << cgicc::input().set ("type", "text").set ("name", "HwDescriptionFile").set ("id", "HwDescriptionFile").set ("size", "70").set ("value", fHWDescriptionFile->toString() ).set ("disabled", "true") << std::endl;
        *out << cgicc::input().set ("type", "submit").set ("title", "change the Hw Description File").set ("value", "Load").set ("disabled", "true") << std::endl;
    }

    *out << cgicc::form() << std::endl;
    *out << cgicc::div() << std::endl;
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
        LOG4CPLUS_ERROR (fLogger, e.what() );
    }

    if (!fHWDescriptionFile->toString().empty() && checkFile (fHWDescriptionFile->toString() ) )
    {
        fHWFormString = XMLUtils::transformXmlDocument (fHWDescriptionFile->toString(), expandEnvironmentVariables (XSLSTYLESHEET), cLogStream);
        cleanup_after_XSLT (fHWFormString);
        fSettingsFormString = XMLUtils::transformXmlDocument (fHWDescriptionFile->toString(), expandEnvironmentVariables (SETTINGSSTYLESHEET), cLogStream);

        if (fSettingsFormString.empty() )
            LOG4CPLUS_ERROR (fLogger, "Error, HW Description File " << fHWDescriptionFile->toString() << " does not contain any run settings!");
    }
    else
        LOG4CPLUS_ERROR (fLogger, "Error, HW Description File " << fHWDescriptionFile->toString() << " is an empty string or does not exist!");

    LOG4CPLUS_INFO (fLogger, cLogStream.str() );
    this->lastPage (in, out);
}

void SupervisorGUI::displayDumpForm (xgi::Input* in, xgi::Output* out)
{
    //string defining action
    std::string url = fURN + "dumpHWDescription";
    *out << cgicc::form().set ("style", "padding-top:10px").set ("style", "padding-bottom:10px").set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data") << std::endl;
    *out << "<label for=\"fileDumpPath\">Path for Hw File dump:  </label>" << std::endl;
    *out << cgicc::input().set ("type", "text").set ("name", "fileDumpPath").set ("title", "file path to dump HW Description File").set ("value", expandEnvironmentVariables (HOME) ).set ("size", "70") << std::endl;
    *out << cgicc::input().set ("type", "submit").set ("title", "dump HWDescription form to XML File").set ("value", "Dump") << std::endl;
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
                LOG4CPLUS_INFO (fLogger, cLogStream.str() );
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
    std::ostringstream cLogStream;

    std::string url = fURN + "handle_Ph2_ACFForm";

    std::string JSfile = expandEnvironmentVariables (HOME);
    JSfile += "/html/formfields.js";

    *out << cgicc::script().set ("type", "text/javascript") << std::endl;
    *out << parseExternalResource (JSfile, cLogStream) << std::endl;
    *out << cgicc::script() << std::endl;
    *out << cgicc::form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data").set ("onclick", "displayACFfields();") << std::endl;
    //left form part
    *out << cgicc::div().set ("class", "acf_left") << std::endl;
    *out << cgicc::fieldset().set ("style", "margin-top:20px") << cgicc::legend ("Ph2_ACF Main Settings") << std::endl;
    *out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", "procedure").set ("value", "Data Taking").set ("checked", "checked") << "Data Taking" << cgicc::label() << cgicc::br() << std::endl;
    *out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", "procedure").set ("value", "Calibration") << "Calibration" << cgicc::label() << cgicc::br() << std::endl;
    *out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", "procedure").set ("value", "Pedestal & Noise") << "Pedestal&Noise" << cgicc::label() << cgicc::br() << std::endl;
    *out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", "procedure").set ("value", "Commission") << "Commission" << cgicc::label() << cgicc::br() << std::endl;
    *out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", "procedure").set ("value", "Pulseshape") << "Pulseshape" << cgicc::label() << cgicc::br() << std::endl;
    *out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", "procedure").set ("value", "Hybridtest") << "Hybridtest" << cgicc::label() << cgicc::br() << std::endl;
    *out << cgicc::label() << cgicc::input().set ("type", "checkbox").set ("name", "procedure").set ("value", "Systemtest") << "Systemtest" << cgicc::label() << cgicc::br() << std::endl;
    *out << cgicc::fieldset() << std::endl;
    *out << cgicc::div() << std::endl;

    //right form part
    *out << cgicc::div().set ("class", "acf_right") << std::endl;

    *out << cgicc::fieldset().set ("style", "margin-top:20px") << cgicc::legend ("Main Run Settings") << std::endl;
    *out << cgicc::table() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::td() << cgicc::label() << "Runnumber: " << cgicc::label() << cgicc::td() << cgicc::td() << cgicc::input().set ("type", "text").set ("name", "runnumber").set ("size", "20").set ("value", fRunNumber->toString() ) << cgicc::td() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::td() << cgicc::label() << "Number of Events: " << cgicc::label() << cgicc::td() << cgicc::td() << cgicc::input().set ("type", "text").set ("name", "nevents").set ("size", "20").set ("placeholder", "number of events to take").set ("value", fNEvents->toString() ) << cgicc::td() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::td() << cgicc::label() << "Result Directory: " << cgicc::label() << cgicc::td() << cgicc::td() << cgicc::input().set ("type", "text").set ("name", "result_directory").set ("size", "50").set ("value", expandEnvironmentVariables (fDirectory->toString() ) )  <<  cgicc::td() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::td() << cgicc::label() << "Data Directory: " << cgicc::label() << cgicc::td() << cgicc::td() << cgicc::input().set ("type", "text").set ("name", "data_directory").set ("size", "50").set ("value", expandEnvironmentVariables (fDirectory->toString() ) )   << cgicc::td() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::table() << std::endl;

    *out << cgicc::fieldset().set ("style", "margin-top:20px").set ("style", "display:none") << cgicc::legend ("Commissioning Settings").set ("style", "display:none") << std::endl;
    *out << cgicc::table().set ("style", "display:none") << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::td() << cgicc::label() << "Latency Scan" << cgicc::input().set ("type", "checkbox").set ("name", "latency").set ("value", "latency").set ("checked", "checked") << cgicc::label() << cgicc::td() << std::endl;
    *out << cgicc::td() << cgicc::label() << "Stub Latency Scan" << cgicc::input().set ("type", "checkbox").set ("name", "stublatency").set ("value", "stublatency") << cgicc::label() << cgicc::td() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::td() << cgicc::label() << "Minimum Latency: " << cgicc::input().set ("type", "text").set ("name", "minimum_latency").set ("size", "5").set ("value", "0") << cgicc::label() << cgicc::td() << std::endl;
    *out << cgicc::td() << cgicc::label() << "Latency Range: " << cgicc::input().set ("type", "text").set ("name", "latency_range").set ("size", "5").set ("value", "10") << cgicc::label() << cgicc::td() << std::endl;
    *out << cgicc::tr() << std::endl;
    *out << cgicc::table() << std::endl;
    *out << cgicc::fieldset() << std::endl;

    *out << cgicc::fieldset() << std::endl;

    *out << cgicc::div() << std::endl;

    //Main Settings parsed from file!
    *out << cgicc::fieldset().set ("style", "margin-top:40px").set ("style", "display:none") << cgicc::legend ("Application Settings").set ("style", "display:none") << std::endl;
    *out << cgicc::table().set ("name", "settings_table").set ("style", "display:none") << std::endl;
    *out << fSettingsFormString << std::endl;
    *out << cgicc::fieldset() << std::endl;
    *out << cgicc::table() << std::endl;
    *out << cgicc::form() << std::endl;

    LOG4CPLUS_INFO (fLogger, cLogStream.str() );
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
        LOG4CPLUS_ERROR (fLogger, e.what() );
    }

    //set this to true once the HWDescription object is initialized to get a reduced set of form input pairs to modify existing HWDescription objects
    bool cStripUnchanged = true;

    *fHWFormData = XMLUtils::updateHTMLForm (this->fHWFormString, cHWFormPairs, cLogStream, cStripUnchanged );

    //for (auto cPair : *fHWFormData)
    //std::cout << cPair.first << " " << cPair.second << std::endl;

    LOG4CPLUS_INFO (fLogger, cLogStream.str() );

    this->lastPage (in, out);
}
