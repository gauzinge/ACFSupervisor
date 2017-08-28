#include "DTCSupervisor/SupervisorGUI.h"

using namespace Ph2TkDAQ;

SupervisorGUI::SupervisorGUI (xdaq::WebApplication* pApp, DTCStateMachine* pStateMachine) :
    fApp (pApp),
    fManager (pApp),
    fFSM (pStateMachine),
    fHWDescriptionFile (nullptr),
    fXLSStylesheet (nullptr),
    fHWFormString ("")
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

    //LOG4CPLUS_INFO (fLogger, cLogStream.str() );
    this->createHtmlFooter (in, out);
}

void SupervisorGUI::ConfigPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    //define view and create header
    fCurrentPageView = Tab::CONFIG;
    this->createHtmlHeader (in, out, fCurrentPageView);

    char cState = fFSM->getCurrentState();

    //string defining action
    std::string url = fURN + "handleHWFormData";

    this->displayLoadForm (in, out);

    // Display the HwDescription HTML form
    // only allow changes in Initial and Configured
    *out << cgicc::form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data") << std::endl;

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
}

void SupervisorGUI::createHtmlHeader (xgi::Input* in, xgi::Output* out, Tab pTab)
{
    // Create the Title, Tab bar
    std::ostringstream cLogStream;

    fManager.getHTMLHeader (in, out);
    //Style this thing
    *out << cgicc::style() << std::endl;
    *out << parseStylesheetCSS (expandEnvironmentVariables ("${DTCSUPERVISOR_ROOT}/html/Stylesheet.css"), cLogStream) << std::endl;
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

        *out << cgicc::br() << std::endl;
        *out << cgicc::div().set ("class", "current_file") << "Current HW File: " << fHWDescriptionFile->toString() << cgicc::div()  << std::endl;
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
        if (cTransition == "Initialise" && fHWFormString.empty() )
        {
            this->reloadHWFile (in, out);

            if (!fHWFormData->size() )
                cLogStream << BOLDYELLOW << "HW Description File " << fHWDescriptionFile->toString() << " not changed by the user - thus initializing from XML" << REST << std::endl;
        }

        // if the transition is Configure, and the HWForm data has not been submitted get the hwForm Data
        if ( cTransition == "Configure" && !fHWFormData->size() )
            cLogStream << BOLDYELLOW << "HW Description File " << fHWDescriptionFile->toString() << " not changed by the user - thus configuring from values parsed from XML" << REST << std::endl;

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

        if (!cHWDescriptionFile.empty() && Ph2TkDAQ::checkFile (cHWDescriptionFile) )
        {
            *fHWDescriptionFile = cHWDescriptionFile;
            cLogStream << BLUE << std::endl << "Changed HW Description File to: " << fHWDescriptionFile->toString() << RESET << std::endl;
        }
        else
            cLogStream << RED << "Error, HW Description File " << cHWDescriptionFile << " is an empty string or does not exist!" << RESET << std::endl;
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (fLogger, e.what() );
    }

    if (checkFile (fHWDescriptionFile->toString() ) )
        fHWFormString = XMLUtils::transformXmlDocument (fHWDescriptionFile->toString(), fXLSStylesheet->toString(), cLogStream);
    else
        cLogStream << RED << "Error, HW Description File " << fHWDescriptionFile->toString() << " is an empty string or does not exist!" << RESET << std::endl;

    LOG4CPLUS_INFO (fLogger, cLogStream.str() );
    this->lastPage (in, out);
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

        //TODO
        //if the above does not work because the client does not sumit the form, I need to parse the html form and get all values manually which is painfull but hey!
        //alternatively I could just configure from the blank xml if the user has not submitted any input! in this case the the internal fFormData of this and the Supervisor is empty - if that is the case, so be it - we just roll with the xml
        //cHWFormPairs = XMLUtils::queryHTMLForm (this->fHWFormString, cLogStream);
    }

    //set this to true once the HWDescription object is initialized to get a reduced set of form input pairs to modify existing HWDescription objects
    bool cStripUnchanged = false;

    if (fFSM->getCurrentState() != 'I') cStripUnchanged = true;

    *fHWFormData = XMLUtils::updateHTMLForm (this->fHWFormString, cHWFormPairs, cLogStream, cStripUnchanged );

    for (auto cPair : *fHWFormData)
        std::cout << cPair.first << " " << cPair.second << std::endl;

    LOG4CPLUS_INFO (fLogger, cLogStream.str() );
    this->lastPage (in, out);
}
