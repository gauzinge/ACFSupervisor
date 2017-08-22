#include "DTCSupervisor/DTCSupervisor.h"

using namespace cgicc;

//constructor
XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)

Ph2TkDAQ::DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::WebApplication (s), // xgi::framework::UIManager (this),
    fHWDescriptionFile (""),
    fXLSStylesheet ("")
{
    //bind xgi and xoap commands to methods
    //methods for tab navigation
    xgi::bind (this, &DTCSupervisor::Default, "Default");
    xgi::bind (this, &DTCSupervisor::MainPage, "MainPage");
    xgi::bind (this, &DTCSupervisor::ConfigPage, "ConfigPage");

    //helper methods for buttons etc
    xgi::bind (this, &DTCSupervisor::loadHWDescriptionFile, "loadHWDescriptionFile");

    //make configurable variapbles available in the Application Info Space
    this->getApplicationInfoSpace()->fireItemAvailable ("HWDescriptionFile", &fHWDescriptionFile);
    this->getApplicationInfoSpace()->fireItemAvailable ("XSLStylesheet", &fXLSStylesheet);
    //detect when default values have been set
    this->getApplicationInfoSpace()->addListener (this, "urn:xdaq-event:setDefaultValues");

    //set the current view to MAIN
    fCurrentPageView = Tab::MAIN;
}

//Destructor
Ph2TkDAQ::DTCSupervisor::~DTCSupervisor() {}

//configure action listener
void Ph2TkDAQ::DTCSupervisor::actionPerformed (xdata::Event& e)
{
    if (e.type() == "urn:xdaq-event:setDefaultValues")
    {
        std::stringstream ss;
        ss << BLUE << "HW Description file: " << fHWDescriptionFile.toString() << " set!" << std::endl;
        ss << "XSL HW Description Stylesheet: " << fXLSStylesheet.toString() << " set!" << std::endl;
        ss << "All Default Values set!" << RESET << std::endl;
        LOG4CPLUS_INFO (this->getApplicationLogger(), ss.str() );
        //here is the listener for FSM state transition commands via xoap
        //have a look at https://gitlab.cern.ch/cms_tk_ph2/BoardSupervisor/blob/master/src/common/BoardSupervisor.cc
        //at a later point!
    }
}

void Ph2TkDAQ::DTCSupervisor::Default (xgi::Input* in, xgi::Output* out)
throw (xgi::exception::Exception)
{
    this->createHtmlHeader (out, fCurrentPageView);
    this->showStateMachineStatus (out);
    this->MainPage (in, out);
}

void Ph2TkDAQ::DTCSupervisor::MainPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    fCurrentPageView = Tab::MAIN;
    this->createHtmlHeader (out, fCurrentPageView);
    this->showStateMachineStatus (out);

    std::string url = "/" + getApplicationDescriptor()->getURN() + "/" + "loadHWDescriptionFile";

    std::ostringstream cForm;

    cForm << cgicc::form().set ("method", "POST").set ("action", url).set ("enctype", "multipart/form-data") << std::endl;
    cForm << "<label for=\"HwDescriptionFile\">HwDescriptionFile: </label>" << std::endl;
    //cForm << "<input type=\"text\" name=\"HwDescriptionFile\" id=\"HwDescriptionFile\" size=\"60\" value=\"" << "/afs/cern.ch/user/g/gauzinge/Ph2_ACF/settings/D19CDescription.xml" << "\"/>" << std::endl;
    cForm << "<input type=\"text\" name=\"HwDescriptionFile\" id=\"HwDescriptionFile\" size=\"60\" value=\"" << Ph2TkDAQ::expandEnvironmentVariables (fHWDescriptionFile.toString () ) << "\"/>" << std::endl;
    cForm << cgicc::input().set ("type", "submit").set ("title", "change the Hw Description File").set ("value", "Load") << std::endl;
    cForm << cgicc::form() << std::endl;

    *out << "<div class=\"content\">" << std::endl;
    *out << cgicc::h3 ("DTCSupervisor Main Page") << std::endl;
    *out << cForm.str() << std::endl;
    *out << "</div>" << std::endl;
}

void Ph2TkDAQ::DTCSupervisor::ConfigPage (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    fCurrentPageView = Tab::CONFIG;
    this->createHtmlHeader (out, fCurrentPageView);
    this->showStateMachineStatus (out);
    std::ostringstream cLogStream;

    // Display the HwDescription HTML form
    *out << "<div class=\"content\">" << std::endl;
    *out << Ph2TkDAQ::transformXmlDocument (expandEnvironmentVariables ("${PH2ACF_ROOT}/settings/D19CDescription.xml"), expandEnvironmentVariables ("${PH2ACF_ROOT}/settings/misc/HWDescription.xsl"), cLogStream) << std::endl;
    *out << "</div>" << std::endl;

    LOG4CPLUS_INFO (this->getApplicationLogger(), cLogStream.str() );
}

void Ph2TkDAQ::DTCSupervisor::createHtmlHeader (xgi::Output* out, Tab pTab, const std::string& strDest)
{
    // Create the Title, Tab bar
    std::ostringstream cLogStream;

    out->getHTTPResponseHeader().addHeader ("Content-Type", "text/html");
    *out << html().set ("lang", "en").set ("dir", "ltr") << std::endl;
    *out << HTMLDoctype (HTMLDoctype::eStrict) << std::endl;

    *out << head() << std::endl;
    //Style this thing
    *out << style() << std::endl;
    *out << Ph2TkDAQ::parseStylesheetCSS (Ph2TkDAQ::expandEnvironmentVariables ("${DTCSUPERVISOR_ROOT}/html/Stylesheet.css"), cLogStream) << std::endl;
    *out << style() << std::endl;

    *out << title ("DTC Supervisor")  << std::endl;
    *out << head() << std::endl;

    std::ostringstream cTabBarString;
    std::string url = "/" + getApplicationDescriptor()->getURN() + "/";

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

    *out << "<div class=\"title\"> <h2> DTC Supervisor </h2></div>" << std::endl;
    *out << "<div class=\"tab\">" << std::endl;
    *out << cTabBarString.str() << std::endl;
    *out << "</div>" << std::endl;

    LOG4CPLUS_INFO (this->getApplicationLogger(), cLogStream.str() );
}

void Ph2TkDAQ::DTCSupervisor::showStateMachineStatus (xgi::Output* out) throw (xgi::exception::Exception)
{
    // create the FSM Status bar showing the current state
    try
    {
        //std::string action = toolbox::toString ("/%s/fsmTransition", getApplicationDescriptor()->getURN().c_str() );

        // display FSM
        //std::set<std::string> possibleInputs = fsm_.getInputs (fsm_.getCurrentState() );
        //std::set<std::string> allInputs = fsm_.getInputs();

        *out << "<div class=\"sidenav\">" << std::endl;
        *out << "Current State: " << "Somestate" << std::endl;
        *out << cgicc::br() << std::endl;
        *out << "<a href=\"#\" class=\"button\"> Initialize </a>" << cgicc::br() << std::endl;
        *out << "<a href=\"#\" class=\"button\"> Configure </a>" << cgicc::br() << std::endl;
        *out << "<a href=\"#\" class=\"button\"> Start </a>" << cgicc::br() << std::endl;
        *out << "<a href=\"#\" class=\"button\"> Stop </a>" << cgicc::br() << std::endl;
        *out << "<a href=\"#\" class=\"button\"> Pause </a>" << cgicc::br() << std::endl;
        *out << "<a href=\"#\" class=\"button\"> Resume </a>" << cgicc::br() << std::endl;
        *out << "<a href=\"#\" class=\"button\"> Halt </a>" << cgicc::br() << std::endl;
        *out << "<a href=\"#\" class=\"button\"> Destroy </a>" << cgicc::br() << std::endl;
        *out << "</div>" << std::endl;
    }
    catch (xgi::exception::Exception& e)
    {
        XCEPT_RETHROW (xgi::exception::Exception, "Exception caught in WebShowRun", e);
    }
}

void Ph2TkDAQ::DTCSupervisor::loadHWDescriptionFile (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    std::ostringstream cLogStream;

    //get the inptu from the form
    cgicc::Cgicc cgi (in);
    std::string cHWDescriptionFile = cgi.getElement ("HwDescriptionFile")->getValue();
    std::cout << cHWDescriptionFile << std::endl;

    if (!cHWDescriptionFile.empty() )
    {
        if (Ph2TkDAQ::checkFile (cHWDescriptionFile) )
        {
            fHWDescriptionFile = cHWDescriptionFile;
            cLogStream << BLUE << "Changed HW Description File to: " << cHWDescriptionFile << RESET << std::endl;
        }
    }
    else
        cLogStream << RED << "Error, HW Description File " << cHWDescriptionFile << " is an empty string or does not exist!" << RESET << std::endl;

    LOG4CPLUS_INFO (this->getApplicationLogger(), cLogStream.str() );
    this->MainPage (in, out);
}
