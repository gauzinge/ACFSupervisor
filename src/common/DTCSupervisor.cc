#include "DTCSupervisor/DTCSupervisor.h"

using namespace Ph2TkDAQ;

//constructor
XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)
INITIALIZE_EASYLOGGINGPP

DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::Application (s),
    fFSM (this),
    fHWDescriptionFile (""),
    fDataDirectory (""),
    fResultDirectory (""),
    fRunNumber (-1),
    fNEvents (0),
    fACFLock (toolbox::BSem::FULL, true), // the second argument is the recursive flag
    fServerPort (8080),
#ifdef __HTTP__
    fHttpServer (nullptr),
#endif
    fSystemController (nullptr),
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

    std::ostringstream oss;
    oss << this->getApplicationDescriptor()->getClassName()
        << this->getApplicationDescriptor()->getInstance();
    fAppNameAndInstanceString = oss.str();
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
    this->getApplicationInfoSpace()->fireItemAvailable ("ShortPause", &fShortPause);
    this->getApplicationInfoSpace()->fireItemAvailable ("THttpServerPort", &fServerPort);

    //bind the action signature for the calibration action and create the workloop
    this->fCalibrationAction = toolbox::task::bind (this, &DTCSupervisor::CalibrationJob, "Calibration");
    this->fCalibrationWorkloop = toolbox::task::getWorkLoopFactory()->getWorkLoop (fAppNameAndInstanceString + "Calibrating", "waiting");

    //bind the action signature for the DAQ action and create the workloop
    this->fDAQAction = toolbox::task::bind (this, &DTCSupervisor::DAQJob, "DAQ");
    this->fDAQWorkloop = toolbox::task::getWorkLoopFactory()->getWorkLoop (fAppNameAndInstanceString + "DAQ", "waiting");

    //detect when default values have been set
    this->getApplicationInfoSpace()->addListener (this, "urn:xdaq-event:setDefaultValues");

    //pass the form data members to the GUI
    fGUI->setHWFormData (&fHWFormData);
    fGUI->setSettingsFormData (&fSettingsFormData);

    //initialize the FSM
    fFSM.initialize<Ph2TkDAQ::DTCSupervisor> (this);

    //configure the logger for the Ph2 ACF libraries
    el::Configurations conf (expandEnvironmentVariables ("${DTCSUPERVISOR_ROOT}/xml/logger.conf") );
    el::Loggers::reconfigureAllLoggers (conf);

    //if defined, create a THttpServer
#ifdef __HTTP__

    if (fHttpServer)
        delete fHttpServer;

    char hostname[HOST_NAME_MAX];

    try
    {
        int cServerPort = fServerPort;
        fHttpServer = new THttpServer ( string_format ( "http:%d", cServerPort ).c_str() );
        fHttpServer->SetReadOnly ( true );
        //fHttpServer->SetTimer ( pRefreshTime, kTRUE );
        fHttpServer->SetTimer (1000, kFALSE);
        fHttpServer->SetJSROOT ("https://root.cern.ch/js/latest/");

        //configure the server
        // see: https://root.cern.ch/gitweb/?p=root.git;a=blob_plain;f=tutorials/http/httpcontrol.C;hb=HEAD
        fHttpServer->SetItemField ("/", "_monitoring", "5000");
        fHttpServer->SetItemField ("/", "_layout", "grid2x2");

        gethostname (hostname, HOST_NAME_MAX);
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), BOLDRED << "Exceptin when trying to start THttpServer: " << e.what() << RESET);
    }

    LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDBLUE << "Opening THttpServer on port " << fServerPort << ". Point your browser to: " << BOLDGREEN << hostname << ":" << fServerPort << RESET);
#else
    LOG4CPLUS_ERROR (this->getApplicationLogger(), BOLDRED << "Error, ROOT version < 5.34 detected or not compiled with Http Server support!"  << " No THttpServer available! - The webgui will fail to show plots!" << RESET);
#endif
}

//Destructor
DTCSupervisor::~DTCSupervisor()
{
    if (fSystemController) delete fSystemController;

    if (fSLinkFileHandler) delete fSLinkFileHandler;
}

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

        fDataDirectory = Ph2TkDAQ::appendSlash (fDataDirectory.toString() );
        fResultDirectory = Ph2TkDAQ::appendSlash (fResultDirectory.toString() );

        fEventCounter = 0;

        //need to nofify the GUI of these variables
        fGUI->fHWDescriptionFile = &fHWDescriptionFile;
        fGUI->fDataDirectory = &fDataDirectory;
        fGUI->fResultDirectory = &fResultDirectory;
        fGUI->fRunNumber = &fRunNumber;
        fGUI->fNEvents = &fNEvents;
        fGUI->fEventCounter = &fEventCounter;
        fGUI->fRAWFile = &fRAWFile;
        fGUI->fDAQFile = &fDAQFile;
        fGUI->fServerPort = &fServerPort;

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
        ss << "ShortPause: " << RED << fShortPause << GREEN << " set!" << std::endl;
        ss << "All Default Values set!" << std::endl;
        ss << BOLDYELLOW <<  "***********************************************************" << RESET << std::endl;
        LOG4CPLUS_INFO (this->getApplicationLogger(), ss.str() );
#ifdef __HTTP__
        fHttpServer->AddLocation ("Calibrations", fResultDirectory.toString().c_str() );
#endif
    }
}

void DTCSupervisor::Default (xgi::Input* in, xgi::Output* out)
throw (xgi::exception::Exception)
{
    std::string name = in->getenv ("PATH_INFO");
    static_cast<xgi::MethodSignature*> (fGUI->getMethod (name) )->invoke (in, out);
}

bool DTCSupervisor::CalibrationJob (toolbox::task::WorkLoop* wl)
{
    try
    {
        fACFLock.take();
#ifdef __HTTP__
        Tool* cTool = new Tool (fHttpServer);
#else
        Tool* cTool = new Tool();
#endif
        cTool->Inherit (fSystemController);
        std::string cResultDirectory = fResultDirectory.toString() + "CommissioningCycle";
        cTool->CreateResultDirectory (cResultDirectory, false, true);
        cTool->InitResultFile ("CommissioningCycle");
        //cTool->StartHttpServer (8080, true);
        fACFLock.give();

        auto cProcedure = fGUI->fProcedureMap.find ("Calibration");

        if (cProcedure != std::end (fGUI->fProcedureMap) && cProcedure->second == true )
        {
            Calibration cCalibration;
            fACFLock.take();
            cCalibration.Inherit (cTool);
            cCalibration.Initialise (false);
            this->updateLogs();
            cCalibration.FindVplus();
            this->updateLogs();
            cCalibration.FindOffsets();
            this->updateLogs();
            cCalibration.writeObjects();
            this->updateLogs();
            cCalibration.dumpConfigFiles();
            fACFLock.give();
            this->updateLogs();
            cProcedure->second = false;
        }

        cProcedure = fGUI->fProcedureMap.find ("Pedestal&Noise");

        if (cProcedure->second == true) //procedure enabled
        {
            PedeNoise cPedeNoise;
            fACFLock.take();
            cPedeNoise.Inherit (cTool);
            cPedeNoise.Initialise();
            this->updateLogs();
            cPedeNoise.measureNoise();
            fACFLock.give();
            this->updateLogs();
            cProcedure->second = false;
        }

        cProcedure = fGUI->fProcedureMap.find ("Commissioning");

        if (cProcedure->second == true) //procedure enabled
        {
            LatencyScan cCommissioning;
            fACFLock.take();
            cCommissioning.Inherit (cTool);
            cCommissioning.Initialize (fGUI->fLatencyStartValue, fGUI->fLatencyRange);
            this->updateLogs();

            if ( fGUI->fLatency ) cCommissioning.ScanLatency ( fGUI->fLatencyStartValue, fGUI->fLatencyRange );

            this->updateLogs();

            if ( fGUI->fStubLatency ) cCommissioning.ScanStubLatency (fGUI->fLatencyStartValue, fGUI->fLatencyRange);

            fACFLock.give();
            this->updateLogs();
            cProcedure->second = false;
        }

        //TODO: check if this does not cause trouble!
        fACFLock.take();
        cTool->SoftDestroy();
        delete cTool;
        fACFLock.give();
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << e.what() << RESET );
        fFSM.fireFailed ("Error in calibration workloop", this);
    }

    fFSM.fireEvent ("Stop", this);
    return false;
}

// will need  a semaphore in order to be able to stop this!
bool DTCSupervisor::DAQJob (toolbox::task::WorkLoop* wl)
{

    // first, check if we already have enough events
    // this requires that we are not taking an infinite number of events
    //if (fNEvents != static_cast<xdata::UnsignedInteger32> (0) && fNEvents < static_cast<xdata::UnsignedInteger32> (fEventCounter) )
    //{
    ////we are basically done and can stop the workloop execution
    ////firing the stop event will call fSystemController->Stop()
    //fFSM.fireEvent ("Stop", this);
    ////this tells the workloop to stop
    //return false;
    //}

    try
    {
        // in here we only call fSystemController->readData() in a non-blocking fashion
        // thats why the last argument to ReadData is true (we can try with false at a later point)
        // the start command is called in enable()
        BeBoard* cBoard = fSystemController->fBoardVector.at (0);
        uint32_t cEventCount = fSystemController->ReadData (cBoard, false );
        fEventCounter += cEventCount;

        if (cEventCount != 0 && fDAQFile)
        {
            const std::vector<Event*>& events = fSystemController->GetEvents ( cBoard );

            for (auto cEvent : events)
            {
                SLinkEvent cSLev =  cEvent->GetSLinkEvent (cBoard);
                fSLinkFileHandler->set (cSLev.getData<uint32_t>() );
            }

        }
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << e.what() << RESET );
        fFSM.fireFailed ("Error in DAQ workloop", this);
    }

    if (fNEvents == static_cast<xdata::UnsignedInteger32> (0) ) // keep running until we say stop!
        return true;

    else if (static_cast<xdata::UnsignedInteger32> (fEventCounter) < fNEvents)
    {
        // we want to stop automatically once we have reached the number of events set in the GUI
        // therefore, as long as the event counter is smaller than the number of requested events, keep going
        return true;
    }
    else
    {
        // this is true only once we have reached the requested number of events from the GUI
        if (fFSM.getCurrentState() == 'E')
            fFSM.fireEvent ("Stop", this);

        return false;
    }

    std::this_thread::sleep_for (std::chrono::milliseconds (fShortPause) );
}

bool DTCSupervisor::initialising (toolbox::task::WorkLoop* wl)
{
    try
    {
        //first, make sure that the event counter is 0 since we are just initializing
        if (fEventCounter != static_cast<xdata::UnsignedInteger32> (0) ) fEventCounter = 0;

        //get the runnumber from the storage file
        int cRunNumber = fRunNumber;
        std::string cRawOutputFile = fDataDirectory.toString() + getDataFileName (expandEnvironmentVariables ("${PH2ACF_ROOT}"), cRunNumber) ;
        fRunNumber = cRunNumber;

        fACFLock.take();

        //construct a system controller object
        if (fSystemController) delete fSystemController;

        fSystemController = new SystemController();

        //first add a filehandler for the raw data to SystemController
        if (fRAWFile)
        {
            std::cout << cRawOutputFile << std::endl;
            bool cExists = Ph2TkDAQ::mkdir (fDataDirectory.toString() );

            if (!cExists)
                LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Data Directory : " << BOLDBLUE << fDataDirectory.toString() << BOLDGREEN << " does not exist - creating!" << RESET);

            fSystemController->addFileHandler ( cRawOutputFile, 'w' );
            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Saving raw data to: " << BOLDBLUE << cRawOutputFile << RESET);
        }

        if (fDAQFile)
        {
            std::string cDAQOutputFile = fDataDirectory.toString();
            std::string cFile = string_format ("run_%04d.daq", cRunNumber);
            cDAQOutputFile += cFile;
            //then add a file handler for the DAQ data
            fSLinkFileHandler = new FileHandler (cDAQOutputFile, 'w');
            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Saving daq data to: " << BOLDBLUE << fSLinkFileHandler->getFilename() << RESET);
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

        fSystemController->InitializeSettings (fGUI->fSettingsXMLString, cPh2LogStream, false);
        fSystemController->InitializeHw (fGUI->fHwXMLString, cPh2LogStream, false);
        fACFLock.give();

        LOG (INFO) << cPh2LogStream.str() ;
        this->updateLogs();
    }

    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << e.what() << RESET );
        fFSM.fireFailed ("Error on initialising", this);
    }

    fFSM.fireEvent ("InitialiseDone", this);
    return false;
}
///Perform configure transition
bool DTCSupervisor::configuring (toolbox::task::WorkLoop* wl)
{

    try
    {
        fACFLock.take();

        //first, make sure that the event counter is 0 since we are just configuring and havent taken any data yet
        if (fEventCounter != static_cast<xdata::UnsignedInteger32> (0) ) fEventCounter = 0;

        //first, check if there were updates to the HWDescription or the settings
        this->updateHwDescription();
        this->updateSettings();
        // once this is checked, configure
        fSystemController->ConfigureHw();
        fACFLock.give();
        this->updateLogs();
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
        fFSM.fireFailed ("Error on configuring", this);
    }

    fFSM.fireEvent ("ConfigureDone", this);
    return false;
}
///Perform enable transition
bool DTCSupervisor::enabling (toolbox::task::WorkLoop* wl)
{
    try
    {
        //first, check if there were updates to the HWDescription or the settings
        fACFLock.take();
        this->updateHwDescription();
        this->updateSettings();
        fACFLock.give();

        //figure out if we are supposed to run any calibrations
        int cEnabledProcedures = 0;
        bool cDataTaking = false;

        for (auto cProcedure : fGUI->fProcedureMap)
        {

            if (cProcedure.second && cProcedure.first != "Data Taking") cEnabledProcedures++;

            if (cProcedure.second && cProcedure.first == "Data Taking") cDataTaking = true;

            if (cProcedure.second)
                LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDYELLOW << "Running " << BOLDBLUE << cProcedure.first << RESET);
        }

        if (cEnabledProcedures != 0) // we are supposed to run calibrations
        {
            //activate the workloop so it's available
            if (!fCalibrationWorkloop->isActive() )
            {
                fCalibrationWorkloop->activate();
                LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDBLUE << "Starting workloop for calibration tasks!" << RESET);

                try
                {
                    fCalibrationWorkloop->submit (fCalibrationAction);
                }
                catch (xdaq::exception::Exception& e)
                {
                    LOG4CPLUS_ERROR (this->getApplicationLogger(), xcept::stdformat_exception_history (e) );
                }
            }
        }

        //we can only start the DAQ workloop here in case there are no calibrations enabled
        //if there are, we need to run the calibration first, let the workloop finish and remove the calibrations from the list of tasks
        //then we can re-enable and the below will be true
        if (cDataTaking && cEnabledProcedures == 0)
        {
            //first, make sure that the event counter is 0 since we are just starting the run
            if (fEventCounter != static_cast<xdata::UnsignedInteger32> (0) ) fEventCounter = 0;

            if (!fDAQWorkloop->isActive() )
            {
                fDAQWorkloop->activate();
                LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDBLUE << "Starting workloop for data taking!" << RESET);

                try
                {
                    //before we do that, we need to start the flow of Triggers
                    fACFLock.take();
                    fSystemController->Start();
                    fACFLock.give();
                    fDAQWorkloop->submit (fDAQAction);
                }
                catch (xdaq::exception::Exception& e)
                {
                    LOG4CPLUS_ERROR (this->getApplicationLogger(), xcept::stdformat_exception_history (e) );
                }
            }

        }
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
        fFSM.fireFailed ("Error on enabling", this);
    }

    fFSM.fireEvent ("EnableDone", this);
    return false;
}
///Perform halt transition
bool DTCSupervisor::halting (toolbox::task::WorkLoop* wl)
{
    try
    {
        if (fDAQWorkloop->isActive() )
        {
            fDAQWorkloop->cancel();
            fACFLock.take();
            fSystemController->Stop();
            fACFLock.give();
        }

        if (fCalibrationWorkloop->isActive() )
            fCalibrationWorkloop->cancel();

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
        fFSM.fireFailed ("Error on Halting", this);
    }

    fFSM.fireEvent ("HaltDone", this);
    return false;
}
///perform pause transition
bool DTCSupervisor::pausing (toolbox::task::WorkLoop* wl)
{
    try
    {
        fACFLock.take();
        fSystemController->Pause();
        fACFLock.give();
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
        fFSM.fireFailed ("Error on pausing", this);
    }

    fFSM.fireEvent ("PauseDone", this);
    return false;
}
///Perform resume transition
bool DTCSupervisor::resuming (toolbox::task::WorkLoop* wl)
{
    try
    {
        fACFLock.take();
        fSystemController->Resume();
        fACFLock.give();
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
        fFSM.fireFailed ("Error on resuming", this);
    }

    fFSM.fireEvent ("ResumeDone", this);
    return false;
}
///Perform stop transition
bool DTCSupervisor::stopping (toolbox::task::WorkLoop* wl)
{
    try
    {
        if (fDAQWorkloop->isActive() )
        {
            fDAQWorkloop->cancel();
            fACFLock.take();
            fSystemController->Stop();
            fACFLock.give();
        }

        if (fCalibrationWorkloop->isActive() )
            fCalibrationWorkloop->cancel();

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
        fFSM.fireFailed ("Error on stopping", this);
    }

    fFSM.fireEvent ("StopDone", this);
    return false;
}
///Perform destroy transition
bool DTCSupervisor::destroying (toolbox::task::WorkLoop* wl)
{
    try
    {
        if (fDAQWorkloop->isActive() )
            fDAQWorkloop->cancel();

        if (fCalibrationWorkloop->isActive() )
            fCalibrationWorkloop->cancel();

        fACFLock.take();
        fSystemController->Destroy();
        delete fSystemController;
        fSystemController = nullptr;
        fACFLock.give();

    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), e.what() );
        fFSM.fireFailed ("Error on destroying", this);
    }

    fFSM.fireEvent ("DestroyDone", this);
    return false;
}

void DTCSupervisor::updateHwDescription()
{
    char cState = fFSM.getCurrentState();

    //if the state is halted or configured, I get to mess with the HWDescription tree!
    if ( (cState == 'c' || cState == 'e')  )
    {
        if (!fHWFormData.empty() )
        {
            BeBoard* cBoard = fSystemController->fBoardVector.at (0);
            //first, clear what we have already initialized
            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDYELLOW << "The HW Description was changed since initialising - thus updating the memory tree!" << RESET);

            for (auto cRegister = fHWFormData.begin(); cRegister != fHWFormData.end();)
            {
                if (cRegister->first.find ("Register_") == 0 && cRegister->first.find ("...") == std::string::npos)
                    this->handleBeBoardRegister (cBoard, cRegister);

                else if (cRegister->first.find ("glob_cbc_reg") != std::string::npos )
                    this->handleGlobalCbcRegister (cBoard, cRegister);

                else if (cRegister->first.find ("Register_...") == 0 )
                    this->handleCBCRegister (cBoard, cRegister);

                else
                    LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "This setting " << GREEN << cRegister->first << RED << " can currently not be changed after initialising! - you can buy cake for Georg and ask him to fix it..." << RESET);

                fHWFormData.erase (cRegister++);
            }
        }
    }
    else
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << "Error, HW Description Tree can only be updated on configure or enable!" << RESET );
        throw std::runtime_error ("This can only be called in Confuring or Enabling");
        return;
    }
}

void DTCSupervisor::updateSettings()
{
    char cState = fFSM.getCurrentState();

    //if the state is halted or configured, I get to mess with the HWDescription tree!
    if ( (cState == 'c' || cState == 'e') )
    {
        if (!fSettingsFormData.empty() )
        {
            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDYELLOW << "The Settings were changed since initialising - thus updating memory!" << RESET);
            //now simply replace SystemController's settings map with the new one!
            fSystemController->fSettingsMap.clear();

            for (auto cPair : fSettingsFormData)
            {
                fSystemController->fSettingsMap[cPair.first] = convertAnyInt (cPair.second.c_str() );
                LOG4CPLUS_INFO (this->getApplicationLogger(), BLUE << "Updating Setting: " << cPair.first << " to " << cPair.second << RESET);
            }
        }
    }
    else
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << "Error, Settings can only be updated on configure or enable!" << RESET );
        throw std::runtime_error ("This can only be called in Confuring or Enabling");
        return;
    }
}
