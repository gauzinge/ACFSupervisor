#include "DTCSupervisor/DTCSupervisor.h"

using namespace Ph2TkDAQ;

//constructor
XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)

DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::WebApplication (s),
    fManager (this),
    fHWDescriptionFile (""),
    fXLSStylesheet ("")
{
    //instance of my GUI object
    //maybe need to pass some parameters
    std::string url = "/" + getApplicationDescriptor()->getURN() + "/";
    fGUI = new SupervisorGUI (&fManager, &this->getApplicationLogger(), url);
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
    //xgi::bind (this, &DTCSupervisor::reloadHWFile, "reloadHWFile");
    //xgi::bind (this, &DTCSupervisor::handleHWFormData, "handleHWFormData");

    //make configurable variapbles available in the Application Info Space
    this->getApplicationInfoSpace()->fireItemAvailable ("HWDescriptionFile", &fHWDescriptionFile);
    this->getApplicationInfoSpace()->fireItemAvailable ("XSLStylesheet", &fXLSStylesheet);
    //detect when default values have been set
    this->getApplicationInfoSpace()->addListener (this, "urn:xdaq-event:setDefaultValues");

    //pass the form data members to the GUI
    fGUI->setHWFormData (&fHWFormData);
    fGUI->setSettingsFormData (&fSettingsFormData);
}

//Destructor
DTCSupervisor::~DTCSupervisor() {}

//configure action listener
void DTCSupervisor::actionPerformed (xdata::Event& e)
{
    if (e.type() == "urn:xdaq-event:setDefaultValues")
    {
        fHWDescriptionFile = Ph2TkDAQ::expandEnvironmentVariables (fHWDescriptionFile.toString() );
        fXLSStylesheet = Ph2TkDAQ::expandEnvironmentVariables (fXLSStylesheet.toString() );

        fHWDescriptionFile = Ph2TkDAQ::removeFilePrefix (fHWDescriptionFile.toString() );
        fXLSStylesheet = Ph2TkDAQ::removeFilePrefix (fXLSStylesheet.toString() );

        //need to nofify the GUI of these variables
        fGUI->fHWDescriptionFile = &fHWDescriptionFile;
        fGUI->fXLSStylesheet = &fXLSStylesheet;

        std::stringstream ss;

        ss << std::endl << BLUE << "HW Description file: " << fHWDescriptionFile.toString() << " set!" << std::endl;
        ss << "XSL HW Description Stylesheet: " << fXLSStylesheet.toString() << " set!" << std::endl;
        ss << "All Default Values set!" << RESET << std::endl;
        LOG4CPLUS_INFO (this->getApplicationLogger(), ss.str() );
        //here is the listener for FSM state transition commands via xoap
        //have a look at https://gitlab.cern.ch/cms_tk_ph2/BoardSupervisor/blob/master/src/common/BoardSupervisor.cc
        //at a later point!
    }
}

void DTCSupervisor::Default (xgi::Input* in, xgi::Output* out)
throw (xgi::exception::Exception)
{
    std::string name = in->getenv ("PATH_INFO");
    static_cast<xgi::MethodSignature*> (fGUI->getMethod (name) )->invoke (in, out);
    fGUI->lastPage (in, out);
}
