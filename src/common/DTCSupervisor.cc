#include "DTCSupervisor/DTCSupervisor.h"

//using namespace cgicc;

//constructor
XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)

Ph2TkDAQ::DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::WebApplication (s),
    fManager (this),
    fHWDescriptionFile (""),
    fXLSStylesheet ("")
{
    //instance of my GUI object
    //maybe need to pass some parameters
    std::string url = "/" + getApplicationDescriptor()->getURN() + "/";
    fGUI = new SupervisorGUI (fManager, url);
    //programatically binld all GUI methods to the Default method of this piece of code
    std::vector<toolbox::lang::Method*> v = fGUI->getMethods();
    std::vector<toolbox::lang::Method*>::iterator cMethod;

    for (cMethod = v.begin(); cMethod != v.end(); cMethod++)
    {
        if ( (*cMethod)->type() == "cgi")
        {
            std::string cMethodName = static_cast<xgi::MethodSignature*> (*cMethod)->name();
            xgi::bind (this, &DTCSupervisor::Default, cMethodName);
        }
    }

    //bind xgi and xoap commands to methods
    //xgi::bind (this, &DTCSupervisor::Default, "Default");

    //helper methods for buttons etc
    xgi::bind (this, &DTCSupervisor::reloadHWFile, "reloadHWFile");
    xgi::bind (this, &DTCSupervisor::handleHWFormData, "handleHWFormData");

    //make configurable variapbles available in the Application Info Space
    this->getApplicationInfoSpace()->fireItemAvailable ("HWDescriptionFile", &fHWDescriptionFile);
    this->getApplicationInfoSpace()->fireItemAvailable ("XSLStylesheet", &fXLSStylesheet);
    //detect when default values have been set
    this->getApplicationInfoSpace()->addListener (this, "urn:xdaq-event:setDefaultValues");
}

//Destructor
Ph2TkDAQ::DTCSupervisor::~DTCSupervisor() {}

//configure action listener
void Ph2TkDAQ::DTCSupervisor::actionPerformed (xdata::Event& e)
{
    if (e.type() == "urn:xdaq-event:setDefaultValues")
    {
        fHWDescriptionFile = Ph2TkDAQ::expandEnvironmentVariables (fHWDescriptionFile.toString() );
        fXLSStylesheet = Ph2TkDAQ::expandEnvironmentVariables (fXLSStylesheet.toString() );

        fHWDescriptionFile = Ph2TkDAQ::removeFilePrefix (fHWDescriptionFile.toString() );
        fXLSStylesheet = Ph2TkDAQ::removeFilePrefix (fXLSStylesheet.toString() );

        //need to nofify the GUI of these variables
        fGUI->fHWDescriptionFile = fHWDescriptionFile;
        fGUI->fXLSStylesheet = fXLSStylesheet;

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
    std::string name = in->getenv ("PATH_INFO");
    static_cast<xgi::MethodSignature*> (fGUI->getMethod (name) )->invoke (in, out);
    fGUI->MainPage (in, out);
}


void Ph2TkDAQ::DTCSupervisor::reloadHWFile (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    // stream for logger
    std::ostringstream cLogStream;

    //parse the form input
    cgicc::Cgicc cgi (in);
    std::string cHWDescriptionFile;
    cgicc::form_iterator cIt = cgi.getElement ("HwDescriptionFile");

    if (!cIt->isEmpty() && cIt != (*cgi).end() )
    {
        cHWDescriptionFile = cIt->getValue();

        //take action
        if (!cHWDescriptionFile.empty() && Ph2TkDAQ::checkFile (cHWDescriptionFile) )
        {
            fHWDescriptionFile = cHWDescriptionFile;
            cLogStream << BLUE << "Changed HW Description File to: " << fHWDescriptionFile.toString() << RESET << std::endl;
            fGUI->fHWFormString = Ph2TkDAQ::XMLUtils::transformXmlDocument (fHWDescriptionFile.toString(), fXLSStylesheet.toString(), cLogStream);
        }
        else
            cLogStream << RED << "Error, HW Description File " << cHWDescriptionFile << " is an empty string or does not exist!" << RESET << std::endl;
    }

    LOG4CPLUS_INFO (this->getApplicationLogger(), cLogStream.str() );
    fGUI->lastPage (in, out);
}

void Ph2TkDAQ::DTCSupervisor::handleHWFormData (xgi::Input* in, xgi::Output* out) throw (xgi::exception::Exception)
{
    fHWFormVector.clear();
    std::cout << fHWFormVector.size() << std::endl;
    //stream for logger
    std::ostringstream cLogStream;

    // get the form input
    cgicc::Cgicc cgi (in);

    for (auto cIt : *cgi)
    {
        if (cIt.getValue() != "")
        {
            //fHWFormVector.push_back (std::make_pair (Ph2TkDAQ::removeDot (cIt.getName() ), cIt.getValue() ) );
            fHWFormVector.push_back (std::make_pair (cIt.getName(), cIt.getValue() ) );
        }
    }

    //set this to true once the HWDescription object is initialized to get a reduced set of form input pairs to modify existing HWDescription objects
    bool cStripUnchanged = true;
    Ph2TkDAQ::XMLUtils::updateHTMLForm (fGUI->fHWFormString, fHWFormVector, cLogStream, cStripUnchanged );

    for (auto cPair : fHWFormVector)
        std::cout << cPair.first << " " << cPair.second << std::endl;

    LOG4CPLUS_INFO (this->getApplicationLogger(), cLogStream.str() );
    fGUI->lastPage (in, out);
}
