#include "DTCSupervisor/DTCSupervisor.h"

using namespace cgicc;

//constructor
XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)

Ph2TkDAQ::DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::WebApplication (s), // xgi::framework::UIManager (this),
    fHWDescriptionFile ("")
{
    //bind xgi and xoap commands to methods
    xgi::bind (this, &DTCSupervisor::Default, "Default");
    //xgi::bind (this, &DTCSupervisor::MainPage, "MainPage");
    //xgi::bind (this, &DTCSupervisor::EditorPage, "Editor");

    //make configurable variapbles available in the Application Info Space
    this->getApplicationInfoSpace()->fireItemAvailable ("HWDescriptionFile", &fHWDescriptionFile);
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
        ss << "HW Description file: " << std::string (fHWDescriptionFile) << " set!" << std::endl;
        ss << "All Default Values set!" << std::endl;
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
    //this->showStateMachineStatus (out);

    LOG4CPLUS_INFO (this->getApplicationLogger(), "Default method called!");
}

void Ph2TkDAQ::DTCSupervisor::createHtmlHeader (xgi::Output* out, Tab pTab, const std::string& strDest)
{
    out->getHTTPResponseHeader().addHeader ("Content-Type", "text/html");
    *out << html().set ("lang", "en").set ("dir", "ltr") << std::endl;
    *out << HTMLDoctype (HTMLDoctype::eStrict) << std::endl;

    *out << head() << std::endl;
    //Style this thing
    *out << style() << std::endl;
    *out << "body {font-family: \"Lato\", sans-serif;\nbackground-color: #f1f1f1\n}" << std::endl;
    *out << "div.title {\noverflow: hidden;\nborder: none solid #ccc;\nbackground-color: inherit;\n}" << std::endl;
    *out << "div.tab {\noverflow: hidden;\nborder: 1px solid #ccc;\nbackground-color: #f1f1f1;\n}" << std::endl;
    *out << "a.button {\nbackground-color: inherit;\nfloat: left;\nborder: none;\noutline: none;\ncursor: pointer;\npadding: 10px 16px;\ntransition:0.3s;\nfont-size:17px;\n}" << std::endl;
    *out << "a.button:hover {\nbackground-color: #ddd;\n}" << std::endl;
    *out << "a.button.active {\nbackground-color: #ccc;\n}" << std::endl;
    *out << style() << std::endl;

    *out << title ("DTC Supervisor")  << std::endl;
    *out << head() << std::endl;

    //*out <<  a ("Visit the XDAQ Web site").set ("href", "http://xdaq.web.cern.ch") << std::endl;
    //*out << a ("Visit the Ph2 ACF Gitlab Repo").set ("href", "http://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF.git") << std::endl;

    std::string cTabBarString;

    switch (pTab)
    {
        case Tab::MAIN:
            cTabBarString = "<a href='MainPage' class=\"button active\">MainPage</a>  <a href='EditorPage' class=\"button\">EditorPage</a>  <a href='CalibrationPage' class=\"button\">CalibrationPage</a>  <a href='DAQPage' class=\"button\">DAQPage</a>";
            break;

            //case Tab::EDITOR:
            //cTabBarString = "<a href='MainPage' class=\"button\">MainPage</a>  <a href='EditorPage' class=\"button active\">EditorPage</a>  <a href='CalibrationPage' class=\"button\">CalibrationPage</a>  <a href='DAQPage' class=\"button\">DAQPage</a>";
            //break;

            //case Tab::CALIBRATION:
            //cTabBarString = "<a href='MainPage' class=\"button\">MainPage</a>  <a href='EditorPage' class=\"button\">EditorPage</a>  <a href='CalibrationPage' class=\"button active\">CalibrationPage</a>  <a href='DAQPage' class=\"button\">DAQPage</a>";
            //break;

            //case Tab::DAQ:
            //cTabBarString = "<a href='MainPage' class=\"button\">MainPage</a>  <a href='EditorPage' class=\"button\">EditorPage</a>  <a href='CalibrationPage' class=\"button\">CalibrationPage</a>  <a href='DAQPage' class=\"button active\">DAQPage</a>";
            //break;
    }

    *out << "<div class=\"title\"> <h3> DTC Supervisor </h3></div>" << std::endl;
    *out << "<div class=\"tab\">" << std::endl;
    *out << cTabBarString << std::endl;
    *out << "</div>" << std::endl;
    *out << cgicc::div() << std::endl;
    *out << body() << std::endl;
    *out << html() << std::endl;

    //xgi::Utils::getPageHeader ( out, "DTCSupervisor", cTabBarString, getApplicationDescriptor()->getContextDescriptor()->getURL(), getApplicationDescriptor()->getURN(), "/xgi/images/Appkication.jpg" );
}
