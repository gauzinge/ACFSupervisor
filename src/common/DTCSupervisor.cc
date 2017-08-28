#include "DTCSupervisor/DTCSupervisor.h"

using namespace Ph2TkDAQ;

//constructor
XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)

DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::WebApplication (s),
    fFSM (this),
    fHWDescriptionFile ("")
    //fXLSStylesheet ("")
{
    //instance of my GUI object
    fGUI = new SupervisorGUI (this, &fFSM);

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
    //this->getApplicationInfoSpace()->fireItemAvailable ("XSLStylesheet", &fXLSStylesheet);
    //detect when default values have been set
    this->getApplicationInfoSpace()->addListener (this, "urn:xdaq-event:setDefaultValues");

    //pass the form data members to the GUI
    fGUI->setHWFormData (&fHWFormData);
    fGUI->setSettingsFormData (&fSettingsFormData);

    //initialize the FSM
    fFSM.initialize<Ph2TkDAQ::DTCSupervisor> (this);
}

//Destructor
DTCSupervisor::~DTCSupervisor() {}

//configure action listener
void DTCSupervisor::actionPerformed (xdata::Event& e)
{
    if (e.type() == "urn:xdaq-event:setDefaultValues")
    {
        fHWDescriptionFile = Ph2TkDAQ::expandEnvironmentVariables (fHWDescriptionFile.toString() );
        //fXLSStylesheet = Ph2TkDAQ::expandEnvironmentVariables (fXLSStylesheet.toString() );

        fHWDescriptionFile = Ph2TkDAQ::removeFilePrefix (fHWDescriptionFile.toString() );
        //fXLSStylesheet = Ph2TkDAQ::removeFilePrefix (fXLSStylesheet.toString() );

        //need to nofify the GUI of these variables
        fGUI->fHWDescriptionFile = &fHWDescriptionFile;
        //fGUI->fXLSStylesheet = &fXLSStylesheet;

        std::stringstream ss;

        ss << std::endl << BLUE << "HW Description file: " << fHWDescriptionFile.toString() << " set!" << std::endl;
        //ss << "XSL HW Description Stylesheet: " << fXLSStylesheet.toString() << " set!" << std::endl;
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

bool DTCSupervisor::initialising (toolbox::task::WorkLoop* wl)
{
    try
    {

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
    }

    //fGUI->handleHWFormData();
    fFSM.fireEvent ("InitialiseDone", this);
    return false;
}
///Perform configure transition
bool DTCSupervisor::configuring (toolbox::task::WorkLoop* wl)
{
    try
    {

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
    }

    //fGUI->handleHWFormData();
    fFSM.fireEvent ("ConfigureDone", this);
    return false;
}
///Perform enable transition
bool DTCSupervisor::enabling (toolbox::task::WorkLoop* wl) {}
///Perform halt transition
bool DTCSupervisor::halting (toolbox::task::WorkLoop* wl)
{
    try
    {

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
    }

    //fGUI->handleHWFormData();
    fFSM.fireEvent ("HaltDone", this);
    return false;
}
///perform pause transition
bool DTCSupervisor::pausing (toolbox::task::WorkLoop* wl) {}
///Perform resume transition
bool DTCSupervisor::resuming (toolbox::task::WorkLoop* wl) {}
///Perform stop transition
bool DTCSupervisor::stopping (toolbox::task::WorkLoop* wl) {}
///Perform destroy transition
bool DTCSupervisor::destroying (toolbox::task::WorkLoop* wl) {}
