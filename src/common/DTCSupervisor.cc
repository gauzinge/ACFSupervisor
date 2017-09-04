#include "DTCSupervisor/DTCSupervisor.h"

using namespace Ph2TkDAQ;

//constructor
XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)
INITIALIZE_EASYLOGGINGPP

DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::WebApplication (s),
    fFSM (this),
    fHWDescriptionFile (""),
    fDataDirectory (""),
    fResultDirectory (""),
    fRunNumber (-1),
    fNEvents (0),
    fSystemController(),
    fSLinkFileHandler (nullptr)
{
    //instance of my GUI object
    fGUI = new SupervisorGUI (this, &fFSM);

    //programatically bind all GUI methods to the Default method of this piece of code
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

    //make configurable variapbles available in the Application Info Space
    this->getApplicationInfoSpace()->fireItemAvailable ("HWDescriptionFile", &fHWDescriptionFile);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataDirectory", &fDataDirectory);
    this->getApplicationInfoSpace()->fireItemAvailable ("ResultDirectory", &fResultDirectory);
    this->getApplicationInfoSpace()->fireItemAvailable ("RunNumber", &fRunNumber);
    this->getApplicationInfoSpace()->fireItemAvailable ("NEvents", &fNEvents);
    this->getApplicationInfoSpace()->fireItemAvailable ("WriteRAW", &fRAWFile);
    this->getApplicationInfoSpace()->fireItemAvailable ("WriteDAQ", &fDAQFile);

    //detect when default values have been set
    this->getApplicationInfoSpace()->addListener (this, "urn:xdaq-event:setDefaultValues");

    //pass the form data members to the GUI
    fGUI->setHWFormData (&fHWFormData);
    fGUI->setSettingsFormData (&fSettingsFormData);

    //initialize the FSM
    fFSM.initialize<Ph2TkDAQ::DTCSupervisor> (this);

    //configure the logger
    el::Configurations conf (expandEnvironmentVariables ("${PH2ACF_ROOT}/settings/logger.conf") );
    el::Loggers::reconfigureAllLoggers (conf);
}

//Destructor
DTCSupervisor::~DTCSupervisor() {}

//configure action listener
void DTCSupervisor::actionPerformed (xdata::Event& e)
{
    if (e.type() == "urn:xdaq-event:setDefaultValues")
    {
        fHWDescriptionFile = Ph2TkDAQ::expandEnvironmentVariables (fHWDescriptionFile.toString() );
        fDataDirectory = Ph2TkDAQ::expandEnvironmentVariables (fDataDirectory.toString() );
        fResultDirectory = Ph2TkDAQ::expandEnvironmentVariables (fResultDirectory.toString() );

        fHWDescriptionFile = Ph2TkDAQ::removeFilePrefix (fHWDescriptionFile.toString() );
        fDataDirectory = Ph2TkDAQ::removeFilePrefix (fDataDirectory.toString() );
        fResultDirectory = Ph2TkDAQ::removeFilePrefix (fResultDirectory.toString() );

        //need to nofify the GUI of these variables
        fGUI->fHWDescriptionFile = &fHWDescriptionFile;
        fGUI->fDataDirectory = &fDataDirectory;
        fGUI->fResultDirectory = &fResultDirectory;
        fGUI->fRunNumber = &fRunNumber;
        fGUI->fNEvents = &fNEvents;
        fGUI->fRAWFile = &fRAWFile;
        fGUI->fDAQFile = &fDAQFile;

        //load the HWFile we have just set - user can always reload it but this is the default for settings and HWDescription
        fGUI->loadHWFile();

        std::stringstream ss;

        ss << std::endl << BOLDYELLOW << "***********************************************************" << std::endl;
        ss <<  GREEN << "HW Description file: " << RED << fHWDescriptionFile.toString() << GREEN << " set!" << std::endl;
        ss << "Write RAW data: " << RED << fRAWFile.toString() << GREEN << " set!" << std::endl;
        ss << "Write DAQ data: " << RED << fDAQFile.toString() << GREEN << " set!" << std::endl;
        ss << "Data Directory: " << RED << fDataDirectory.toString() << GREEN << " set!" << std::endl;
        ss << "Result Directory: " << RED << fResultDirectory.toString() << GREEN << " set!" << std::endl;
        ss << "Run Number: " << RED << fRunNumber << GREEN << " set!" << std::endl;
        ss << "N Events: " << RED << fNEvents << GREEN << " set!" << std::endl;
        ss << "All Default Values set!" << std::endl;
        ss << BOLDYELLOW <<  "***********************************************************" << RESET << std::endl;
        LOG4CPLUS_INFO (this->getApplicationLogger(), ss.str() );
        //here is the listener for FSM state transition commands via xoap??
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
        fGUI->fAutoRefresh = true;
        //get the runnumber from the storage file
        int cRunNumber = fRunNumber;
        std::string cRawOutputFile = fDataDirectory.toString() + "/" + getDataFileName (expandEnvironmentVariables ("${PH2ACF_ROOT}"), cRunNumber) ;
        fRunNumber = cRunNumber;

        //first add a filehandler for the raw data to SystemController
        if (fRAWFile)
            fSystemController.addFileHandler ( cRawOutputFile, 'w' );

        if (fDAQFile)
        {
            std::string cDAQOutputFile = fDataDirectory.toString() + "/";
            std::string cFile = string_format ("run_%04d.daq", cRunNumber);
            cDAQOutputFile += cFile;
            //then add a file handler for the DAQ data
            fSLinkFileHandler = new FileHandler (cDAQOutputFile, 'w');
        }

        //try to initialize the HWDescription from the GUI's fXMLHWString
        //make sure that the address_table_string is expanded with the correct environment variable

        //if we don't find an environment variable in the path to the address tabel, we prepend the Ph2ACF ROOT
        if (fGUI->fHwXMLString.find ("file:://${") == std::string::npos)
        {
            std::string cCorrectPath = "file://" + expandEnvironmentVariables ("${PH2ACF_ROOT}") + "/";
            cleanupHTMLString (fGUI->fHwXMLString, "file://", cCorrectPath);
        }

        //the same goes for the CBC File Path
        if (fGUI->fHwXMLString.find ("<CBC_Files path=\"${") == std::string::npos)
        {
            std::string cCorrectPath = "<CBC_Files path=\"" + expandEnvironmentVariables ("${PH2ACF_ROOT}");
            cleanupHTMLString (fGUI->fHwXMLString, "<CBC_Files path=\".", cCorrectPath);
        }

        //this is a temporary solution to keep the logs - in fact I should tail the logfile that I have to configure properly before
        std::ostringstream cPh2LogStream;

        fSystemController.InitializeSettings (fGUI->fSettingsXMLString, cPh2LogStream, false);
        fSystemController.InitializeHw (fGUI->fHwXMLString, cPh2LogStream, false);

        std::string cTmpStr = cPh2LogStream.str();
        cleanup_log_string (cTmpStr);
        fGUI->fPh2_ACFLog += cTmpStr;
        std::cout << cPh2LogStream.str() << std::endl;

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << e.what() << RESET );
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
        this->updateHwDescription();

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
bool DTCSupervisor::enabling (toolbox::task::WorkLoop* wl)
{
    try
    {

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
    }

    //fGUI->handleHWFormData();
    fFSM.fireEvent ("EnableDone", this);
    return false;
}
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
bool DTCSupervisor::pausing (toolbox::task::WorkLoop* wl)
{
    try
    {

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
    }

    //fGUI->handleHWFormData();
    fFSM.fireEvent ("PauseDone", this);
    return false;
}
///Perform resume transition
bool DTCSupervisor::resuming (toolbox::task::WorkLoop* wl)
{
    try
    {

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
    }

    //fGUI->handleHWFormData();
    fFSM.fireEvent ("ResumeDone", this);
    return false;
}
///Perform stop transition
bool DTCSupervisor::stopping (toolbox::task::WorkLoop* wl)
{
    try
    {

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
    }

    //fGUI->handleHWFormData();
    fFSM.fireEvent ("StopDone", this);
    return false;
}
///Perform destroy transition
bool DTCSupervisor::destroying (toolbox::task::WorkLoop* wl)
{
    try
    {

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
    }

    //fGUI->handleHWFormData();
    fFSM.fireEvent ("DestroyDone", this);
    return false;
}

void DTCSupervisor::updateHwDescription()
{
    char cState = fFSM.getCurrentState();

    //if the state is halted or configured, I get to mess with the HWDescription tree!
    if ( (cState == 'c' || cState == 'e') && !fHWFormData.empty() )
    {
        //now start messing
        if (fSystemController.fBoardVector.size() )
        {
            BeBoard* cBoard = fSystemController.fBoardVector.at (0);

            //first, handle the BeBoard Registers as these are easiest
            std::vector < std::pair<std::string, uint32_t>> cRegisters = this->getBeBoardRegisters();

            for (auto cPair : cRegisters )
                cBoard->setReg (cPair.first, cPair.second);

            if (cState == 'e')
                fSystemController.fBeBoardInterface->WriteBoardMultReg (cBoard, cRegisters);

            this->handleCBCRegisters (cBoard, cState);
            //next, the condition data - this does not have any influence on configuration or whatsoever, so i can change it in configuring or enabling
            this->handleCondtionData (cBoard );
        }

    }
    else
    {
        throw std::runtime_error ("This can only be called in Confuring or Enabling");
        LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << "Error, HW Description Tree can only be updated on configure or enable!" << RESET );
        return;
    }
}
