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
    fSLinkFileHandler (nullptr),
    fGetRunnumberFromFile (false),
    fPlaybackEventSize32 (0)
{
    //instance of my GUI object
    fGUI = new SupervisorGUI (this, &fFSM);

    //instance of TCP Data Sender
    fDataSender = new TCPDataSender (this->getApplicationLogger() );

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

    //Data to EVM
    this->getApplicationInfoSpace()->fireItemAvailable ("SendData", &fSendData);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataSourceHost", &fSourceHost);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataSourcePort", &fSourcePort);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataSinkHost", &fSinkHost);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataSinkPort", &fSinkPort);

    //Playback Mode
    this->getApplicationInfoSpace()->fireItemAvailable ("PlaybackMode", &fPlaybackMode);
    this->getApplicationInfoSpace()->fireItemAvailable ("PlaybackFile", &fPlaybackFile);

    //bind the action signature for the calibration action and create the workloop
    this->fCalibrationAction = toolbox::task::bind (this, &DTCSupervisor::CalibrationJob, "Calibration");
    this->fCalibrationWorkloop = toolbox::task::getWorkLoopFactory()->getWorkLoop (fAppNameAndInstanceString + "Calibrating", "waiting");

    //bind the action signature for the DAQ action and create the workloop
    this->fDAQAction = toolbox::task::bind (this, &DTCSupervisor::DAQJob, "DAQ");
    this->fDAQWorkloop = toolbox::task::getWorkLoopFactory()->getWorkLoop (fAppNameAndInstanceString + "DAQ", "waiting");

    //bind the action signature for the DS action and create the workloop
    this->fDSAction = toolbox::task::bind (this, &DTCSupervisor::SendDataJob, "DS");
    this->fDSWorkloop = toolbox::task::getWorkLoopFactory()->getWorkLoop (fAppNameAndInstanceString + "DS", "waiting");

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
#ifdef __ROOT6__
        fHttpServer = new THttpServer ( string_format ( "http:%d?cors", cServerPort ).c_str() );
        //fHttpServer = new THttpServer ( string_format ( "http:%d", cServerPort ).c_str() );
#else
        fHttpServer = new THttpServer ( string_format ( "http:%d", cServerPort ).c_str() );
#endif
        fHttpServer->SetReadOnly ( true );
        //fHttpServer->SetTimer ( pRefreshTime, kTRUE );
        fHttpServer->SetTimer (1000, kFALSE);
        fHttpServer->SetJSROOT ("https://root.cern.ch/js/latest/");

        //configure the server
        // see: https://root.cern.ch/gitweb/?p=root.git;a=blob_plain;f=tutorials/http/httpcontrol.C;hb=HEAD
        //fHttpServer->SetItemField ("/", "_monitoring", "5000");
        //fHttpServer->SetItemField ("/", "_layout", "grid2x2");

        gethostname (hostname, HOST_NAME_MAX);
    }
    catch (std::exception& e)
    {
        LOG4CPLUS_ERROR (this->getApplicationLogger(), BOLDRED << "Exceptin when trying to start THttpServer: " << e.what() << RESET);
    }

    LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDBLUE << "Opening THttpServer on port " << fServerPort << ". Point your browser to: " << BOLDGREEN << hostname << ":" << fServerPort << RESET);
    fGUI->fHostString = std::string (hostname) + ":" + std::to_string (fServerPort);
#else
    LOG4CPLUS_ERROR (this->getApplicationLogger(), BOLDRED << "Error, ROOT version < 5.34 detected or not compiled with Http Server support!"  << " No THttpServer available! - The webgui will fail to show plots!" << RESET);
#endif
}

//Destructor
DTCSupervisor::~DTCSupervisor()
{
    if (fSystemController) delete fSystemController;

    if (fSLinkFileHandler) delete fSLinkFileHandler;

    if (fDataSender) delete fDataSender;
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

        fPlaybackFile = Ph2TkDAQ::expandEnvironmentVariables (fPlaybackFile.toString() );
        fPlaybackFile = Ph2TkDAQ::removeFilePrefix (fPlaybackFile.toString() );

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

        fGUI->fSendData = &fSendData;
        fGUI->fSourceHost = &fSourceHost;
        fGUI->fSourcePort = &fSourcePort;
        fGUI->fSinkHost = &fSinkHost;
        fGUI->fSinkPort = &fSinkPort;

        fGUI->fPlaybackMode = &fPlaybackMode;
        fGUI->fPlaybackFile = &fPlaybackFile;

        //load the HWFile we have just set - user can always reload it but this is the default for settings and HWDescription
        fGUI->loadHWFile();

        std::stringstream ss;

        ss << std::endl << BOLDYELLOW << "***********************************************************" << std::endl;
        ss <<  GREEN << "HW Description file: " << RED << fHWDescriptionFile.toString() << GREEN << " set!" << std::endl;
        ss << "Write RAW data: " << RED << fRAWFile.toString() << GREEN << " set!" << std::endl;
        ss << "Write DAQ data: " << RED << fDAQFile.toString() << GREEN << " set!" << std::endl;
        ss << "Send DAQ data: " << RED << fSendData.toString() << GREEN << " set!" << std::endl;

        if (fSendData)
        {
            ss << "Data Source: " << RED << fSourceHost.toString() << ":" << fSourcePort.toString() << GREEN << " set!" << std::endl;
            ss << "Data Sink: " << RED << fSinkHost.toString() << ":" << fSinkPort.toString() << GREEN << " set!" << std::endl;
        }

        if (fPlaybackMode)
            ss << "Playback File: " << RED << fPlaybackFile.toString() << GREEN << " set!" << std::endl;


        ss << "Data Directory: " << RED << fDataDirectory.toString() << GREEN << " set!" << std::endl;
        ss << "Result Directory: " << RED << fResultDirectory.toString() << GREEN << " set!" << std::endl;
        ss << "Run Number: " << RED << fRunNumber << GREEN << " set!" << std::endl;
        ss << "N Events: " << RED << fNEvents << GREEN << " set!" << std::endl;
        ss << "ShortPause: " << RED << fShortPause << GREEN << " set!" << std::endl;
        ss << "All Default Values set!" << std::endl;
        ss << BOLDYELLOW <<  "***********************************************************" << RESET << std::endl;
        LOG4CPLUS_INFO (this->getApplicationLogger(), ss.str() );

        //TODO: Debug
#ifdef __HTTP__
        fHttpServer->AddLocation ("Calibrations/", "/afs/cern.ch/user/g/gauzinge/DTCSupervisor/Results/CommissioningCycle_06-09-17_19:00/" );
        fGUI->appendResultFiles ("http://cmsuptrackerdaq.cern.ch:8080/Calibrations/CommissioningCycle.root");
#endif

        //set the source and sink for the TCP data sender
        fDataSender->setSource (fSourceHost.toString(), fSourcePort);
        fDataSender->setSink (fSinkHost.toString(), fSinkPort);
        fGUI->fDataSenderTable = fDataSender->generateStatTable();
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
#ifdef __HTTP__
        fHttpServer->AddLocation ("latest/", cResultDirectory.c_str() );
        fGUI->appendResultFiles ("http://cmsuptrackerdaq.cern.ch:8080/Calibrations/CommissioningCycle.root");
        //fGUI->appendResultFiles (cTool->getResultFileName() );
#endif
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
        //#ifdef __HTTP__
        //int cLen = 0;
        //fHttpServer->ReadFileContent (cResultFileName.c_str(), cLen );
        //#endif
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
    try
    {
        // in here we only call fSystemController->readData() in a non-blocking fashion
        // thats why the last argument to ReadData is false
        // the start command is called in enable()
        BeBoard* cBoard = fSystemController->fBoardVector.at (0);
        uint32_t cEventCount = fSystemController->ReadData (cBoard, false );
        fEventCounter += cEventCount;

        if (cEventCount != 0 && (fDAQFile || fSendData) )
        {
            const std::vector<Event*>& events = fSystemController->GetEvents ( cBoard );
            std::vector<SLinkEvent> cSLinkEventVec;

            for (auto cEvent : events)
            {
                SLinkEvent cSLev =  cEvent->GetSLinkEvent (cBoard);
                cSLinkEventVec.push_back (cSLev);

                if (fDAQFile)
                    fSLinkFileHandler->set (cSLev.getData<uint32_t>() );
            }

            if (fSendData)
                fDataSender->enqueueEvent (cSLinkEventVec);
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

bool DTCSupervisor::SendDataJob (toolbox::task::WorkLoop* wl)
{
    fGUI->fDataSenderTable = fDataSender->generateStatTable();
    return this->fDataSender->sendData ();
}

bool DTCSupervisor::PlaybackJob (toolbox::task::WorkLoop* wl)
{
    uint32_t cNEvents = 10;

    //super simple, either call cSystemController::readFile() with the appropriate number of chunks or just read the binary data stream until an event trailer is reached
    if (fPlaybackEventSize32 != 0 ) // we are dealing with .raw data
    {
        fACFLock.take();
        BeBoard* cBoard = fSystemController->fBoardVector.at (0);
        std::vector<uint32_t> cDataVec;
        fSystemController->readFile (cDataVec, cNEvents * fPlaybackEventSize32);

        if (cDataVec.size() != 0) // there is still some data in the file
        {
            size_t cCalcEventSize = cDataVec.size() / fPlaybackEventSize32;
            fSystemController->setData (cBoard, cDataVec, cCalcEventSize);

            fEventCounter += cCalcEventSize;

            if (fDAQFile || fSendData)
            {
                const std::vector<Event*>& events = fSystemController->GetEvents ( cBoard );
                std::vector<SLinkEvent> cSLinkEventVec;

                for (auto cEvent : events)
                {
                    SLinkEvent cSLev =  cEvent->GetSLinkEvent (cBoard);
                    cSLinkEventVec.push_back (cSLev);

                    if (fDAQFile)
                        fSLinkFileHandler->set (cSLev.getData<uint32_t>() );
                }

                if (fSendData)
                    fDataSender->enqueueEvent (cSLinkEventVec);
            }

            return true;
        }
        else
        {
            // this is true only once we have reached the end of the file
            if (fFSM.getCurrentState() == 'E')
                fFSM.fireEvent ("Stop", this);

            return false;
        }

        fACFLock.give();
    }
    else //  we are dealing with .daq data that we want to send
    {
        //read the uint64_t vector from ifstream (fPlaybackIfstream)
        std::vector<SLinkEvent> cSLinkEventVec = this->readSLinkFromFile (cNEvents);

        //for (auto cEvent : cSLinkEventVec)
        //cEvent.print();
        if (cSLinkEventVec.size() != 0)
        {
            fEventCounter += cSLinkEventVec.size();

            // still some data in the file
            if (fSendData)
                fDataSender->enqueueEvent (cSLinkEventVec);

            return true;
        }
        else
        {
            // this is true only once we have reached the end of the file
            if (fFSM.getCurrentState() == 'E')
                fFSM.fireEvent ("Stop", this);

            return false;
        }
    }
}

bool DTCSupervisor::initialising (toolbox::task::WorkLoop* wl)
{
    try
    {
        //first, make sure that the event counter is 0 since we are just initializing
        if (fEventCounter != static_cast<xdata::UnsignedInteger32> (0) ) fEventCounter = 0;

        // only if we are not playing back data the below is necessary - otherwise the HW Structure is loaded on configuring
        //if (!fPlaybackMode)
        //{
        fACFLock.take();

        //construct a system controller object
        if (fSystemController) delete fSystemController;

        fSystemController = new SystemController();

        //expand all file paths from HW Description xml string
        complete_file_paths (fGUI->fHwXMLString);

        //this is a temporary solution to keep the logs - in fact I should tail the logfile that I have to configure properly before
        std::ostringstream cPh2LogStream;

        fSystemController->InitializeSettings (fGUI->fSettingsXMLString, cPh2LogStream, false);
        fSystemController->InitializeHw (fGUI->fHwXMLString, cPh2LogStream, false);
        fACFLock.give();

        LOG (INFO) << cPh2LogStream.str() ;
        this->updateLogs();
        //}
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
        if (!fPlaybackMode)
        {
            //first check what we are actually configuring
            for (auto cProcedure : fGUI->fProcedureMap)
            {
                //if we are going to take data, increment the run number if we are getting it from file
                if (cProcedure.second && cProcedure.first == "Data Taking")
                {
                    int cRunNumber = fRunNumber;

                    if (fRunNumber == -1) fGetRunnumberFromFile = true;

                    //get the runnumber from the storage file in Ph2_ACF
                    if (fGetRunnumberFromFile) // this means we want the run number from the file
                    {
                        getRunNumber (expandEnvironmentVariables ("${PH2ACF_ROOT}"), cRunNumber, true) ;
                        fRunNumber = cRunNumber;
                    }

                    std::string cRawOutputFile = fDataDirectory.toString() + string_format ("run%05d.raw", cRunNumber);

                    //first add a filehandler for the raw data to SystemController
                    if (fRAWFile)
                    {
                        bool cExists = Ph2TkDAQ::mkdir (fDataDirectory.toString() );

                        if (!cExists)
                            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Data Directory : " << BOLDBLUE << fDataDirectory.toString() << BOLDGREEN << " does not exist - creating!" << RESET);

                        fSystemController->addFileHandler ( cRawOutputFile, 'w' );
                        fSystemController->initializeFileHandler();
                        LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Saving raw data to: " << BOLDBLUE << cRawOutputFile << RESET);
                    }

                }
            }
        } //end of !fPlayback
        else if (fPlaybackMode)
        {
            if (!Ph2TkDAQ::checkFile (fPlaybackFile.toString() ) )
            {
                LOG4CPLUS_ERROR (this->getApplicationLogger(), BOLDRED << "Error, " << fPlaybackFile.toString() << " does not exist!" << RESET);
                throw std::runtime_error ("Playback Data file does not exist!");
                fFSM.fireEvent ("Destroy", this);
                return false;
            }

            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Configuring applicaton to run in playback mode with playback file: " << BOLDBLUE << fPlaybackFile.toString() << RESET);
            //we are in playback mode, so things should be as simple as possible - we get the runnumber either from the filename or from Ph2_ACF storage file
            //and we also repurpose the system controller file handler to read back the data
            //also, we only want to create the Playback Workloop here since it is not required under normal conditions

            //bind the action signature for the DS action and create the workloop
            this->fPlaybackAction = toolbox::task::bind (this, &DTCSupervisor::PlaybackJob, "Playback");
            this->fPlaybackWorkloop = toolbox::task::getWorkLoopFactory()->getWorkLoop (fAppNameAndInstanceString + "Playback", "waiting");

            int cRunNumber = fRunNumber;

            //if the run number = -1 (means we don't get it from RCMS) we should first check if there is a run number in the filename
            if (fRunNumber == -1)
            {
                if (Ph2TkDAQ::find_run_number (fPlaybackFile.toString(), cRunNumber) )
                    fRunNumber = cRunNumber;

                else
                {
                    getRunNumber (expandEnvironmentVariables ("${PH2ACF_ROOT}"), cRunNumber, true) ;
                    fRunNumber = cRunNumber;
                }
            }

            //here we need to check what kind of file we are playing back - in case of .daq (SLink data) we only read from binary file, so it's easy
            //if we are reading .raw data, we need to get the info from the File Header and compare against the config file, issue a warning and if necessary update the HWDescription
            if (fPlaybackFile.toString().find (".raw") != std::string::npos) // we are dealing with a .raw file
            {
                //add a read file handler to our SystemController, this does not require a call to initializeFileHandler
                fSystemController->addFileHandler ( fPlaybackFile.toString().c_str(), 'r' );

                //now read the file header and check if it makes sense
                if (fSystemController->fFileHandler->fHeaderPresent)
                {
                    LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Found valid File header in " << BOLDBLUE << fPlaybackFile.toString() << RESET);
                    //we have found a reasonable fileHeader - the above statement beeing true also implies a valid header
                    FileHeader cHeader = fSystemController->fFileHandler->getHeader();
                    //now analyze it and check against the HWDescription
                    //in this case we have all the info we need: the FW type, the event size 32 to read in chunks of 100 events and the number of CBCs, so wipe the HW Description tree and populate with what we know
                    BeBoard* cBoard = fSystemController->fBoardVector.at (0);
                    //first, set the board and event type, then modify the NCbc in the vector - for now assuming 1 FE //TODO
                    //the below ensures we have the right Event Object that is used when calling Data.set()
                    cBoard->setBoardType (cHeader.getBoardType() );
                    cBoard->setEventType (cHeader.fEventType);
                    fPlaybackEventSize32 = cHeader.fEventSize32;
                    Counter cCounter;
                    cBoard->accept (cCounter);

                    if (cHeader.fNCbc != cCounter.getNCbc() )
                    {
                        //we have an issue here so output the number of CBCs from the file and the number in the HW Description and fire the Destroy event
                        LOG4CPLUS_ERROR (this->getApplicationLogger(), BOLDRED << "Error, " << fPlaybackFile.toString() << " Contains data from " << BOLDYELLOW << cHeader.fNCbc << BOLDRED << " Cbcs whereas the HWDescription file has " << BOLDBLUE << cCounter.getNCbc() << " Cbcs on " << cCounter.getNFe() << BOLDRED << " Front Ends - I am firing the Destroy state transition and ask you to change the HWDescription file " << BOLDGREEN << fHWDescriptionFile.toString() << BOLDRED << " accordingly!" << RESET);
                        fFSM.fireEvent ("Destroy", this);
                        return false;
                    }

                }
                else
                {
                    //also need to compute event size 32
                    fPlaybackEventSize32 = fSystemController->computeEventSize32 (fSystemController->fBoardVector.at (0) );
                    LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDRED << "Did not find a valid File header in " << BOLDBLUE << fPlaybackFile.toString() << BOLDRED << " - thus trying with info from HWDescription file!" << RESET);
                }
            }
            else if (fPlaybackFile.toString().find (".daq") != std::string::npos) // we are dealing with a .daq file
            {
                //here we just need to open the ifstream
                fPlaybackIfstream.open (fPlaybackFile.toString().c_str(), std::fstream::in |  std::fstream::binary );

                if (fPlaybackIfstream.fail() )
                {
                    LOG4CPLUS_ERROR (this->getApplicationLogger(), BOLDRED << "Error, " << fPlaybackFile.toString() << " could not be opened!" << RESET);
                    throw std::runtime_error ("Could not open playback Data file!");
                    fFSM.fireEvent ("Destroy", this);
                    return false;
                }
                else
                    LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Successfully opened " << fPlaybackFile.toString() << RESET);
            }
            else
            {
                LOG4CPLUS_ERROR (this->getApplicationLogger(), BOLDRED << "Error, " << fPlaybackFile.toString() << " is not a recognized file type: types should either be .daq or .raw" << RESET);
                throw std::runtime_error ("Playback file does not have .raw or .daq extension");
                fFSM.fireEvent ("Destroy", this);
                return false;
            }
        }

        //open the connection for the TCP Data  Sender
        if (fSendData)
        {
            //TODO
            //fDataSender->openConnection();
            fGUI->fDataSenderTable = fDataSender->generateStatTable();
        }

        //then, if required a file handler for the .daq data
        if (fDAQFile)
        {
            int cRunNumber = fRunNumber;
            std::string cDAQOutputFile = fDataDirectory.toString();
            std::string cFile = string_format ("run_%05d.daq", cRunNumber);
            cDAQOutputFile += cFile;
            //then add a file handler for the DAQ data
            fSLinkFileHandler = new FileHandler (cDAQOutputFile, 'w');
            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Saving daq data to: " << BOLDBLUE << fSLinkFileHandler->getFilename() << RESET);
        }


        fACFLock.take();

        //first, make sure that the event counter is 0 since we are just configuring and havent taken any data yet
        if (fEventCounter != static_cast<xdata::UnsignedInteger32> (0) ) fEventCounter = 0;

        if (!fPlaybackMode)
        {
            //first, check if there were updates to the HWDescription or the settings
            this->updateHwDescription();
            this->updateSettings();
            // once this is checked, configure
            fSystemController->ConfigureHw();
        }

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
        if (!fPlaybackMode)
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
                //if SendData is enabled, here we need to start the SendData workloop
                if (fSendData)
                {
                    if (!fDSWorkloop->isActive() )
                    {
                        fDSWorkloop->activate();
                        LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Starting workloop for data sending!" << RESET);

                        fDataSender->ResetTimer();
                        fDataSender->StartTimer();

                        try
                        {
                            fDSWorkloop->submit (fDSAction);
                        }
                        catch (xdaq::exception::Exception& e)
                        {
                            LOG4CPLUS_ERROR (this->getApplicationLogger(), xcept::stdformat_exception_history (e) );
                        }

                        fGUI->fDataSenderTable = fDataSender->generateStatTable();
                    }

                }

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
        else if (fPlaybackMode)
        {
            //here we need to do a couple of things
            //if SendData is enabled, we need to start the SendData workloop
            if (fSendData)
            {
                if (!fDSWorkloop->isActive() )
                {
                    fDSWorkloop->activate();
                    LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Starting workloop for data sending!" << RESET);

                    fDataSender->ResetTimer();
                    fDataSender->StartTimer();

                    try
                    {
                        fDSWorkloop->submit (fDSAction);
                    }
                    catch (xdaq::exception::Exception& e)
                    {
                        LOG4CPLUS_ERROR (this->getApplicationLogger(), xcept::stdformat_exception_history (e) );
                    }

                }

                fGUI->fDataSenderTable = fDataSender->generateStatTable();
            }

            //now, make sure that the event counter is 0 since we are just starting the playback run
            if (fEventCounter != static_cast<xdata::UnsignedInteger32> (0) ) fEventCounter = 0;

            //lastly, start the playback workloop
            if (!fPlaybackWorkloop->isActive() )
            {
                fPlaybackWorkloop->activate();
                LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDBLUE << "Starting workloop for data playback!" << RESET);
            }

            try
            {
                fPlaybackWorkloop->submit (fPlaybackAction);
            }
            catch (xdaq::exception::Exception& e)
            {
                LOG4CPLUS_ERROR (this->getApplicationLogger(), xcept::stdformat_exception_history (e) );
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
        if (!fPlaybackMode)
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
        else if (fPlaybackMode)
        {
            if (fPlaybackWorkloop->isActive() )
                fPlaybackWorkloop->cancel();
        }

        //reset the internal runnumber so we pick up on it on the next initialise
        if (fGetRunnumberFromFile)
            fRunNumber = -1;

        if (fSendData)
        {
            if (fDSWorkloop->isActive() )
                fDSWorkloop->cancel();

            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Data Sender Workloop processed " << fDataSender->getNEventsProcessed() << "Events with a frequencey of " << fDataSender->getEventFrequency() << "Hz" << RESET);

            //TODO
            //fDataSender->closeConnection();
            fGUI->fDataSenderTable = fDataSender->generateStatTable();
        }
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
        if (!fPlaybackMode)
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
        else if (fPlaybackMode)
        {
            if (fPlaybackWorkloop->isActive() )
                fPlaybackWorkloop->cancel();
        }

        if (fSendData)
        {
            fDataSender->StopTimer();
            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Data Sender Workloop processed " << fDataSender->getNEventsProcessed() << "Events with a frequencey of " << fDataSender->getEventFrequency() << "Hz" << RESET );
        }
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

        if (fSendData)
        {
            if (fDSWorkloop->isActive() )
                fDSWorkloop->cancel();
        }

        if (fPlaybackMode)
            if ( fPlaybackWorkloop->isActive() )
                fPlaybackWorkloop->cancel();

        fACFLock.take();
        fSystemController->Destroy();
        delete fSystemController;
        fSystemController = nullptr;
        fACFLock.give();
        fGUI->fHwXMLString.clear();
        fGUI->fSettingsXMLString.clear();
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
