#include "DTCSupervisor/DTCSupervisor.h"

using namespace Ph2TkDAQ;

//constructor
XDAQ_INSTANTIATOR_IMPL (Ph2TkDAQ::DTCSupervisor)
INITIALIZE_EASYLOGGINGPP

DTCSupervisor::DTCSupervisor (xdaq::ApplicationStub* s)
throw (xdaq::exception::Exception) : xdaq::Application (s),
    fFSM (this),
    fFedID (51),
    fHWDescriptionFile (""),
    fDataDirectory (""),
    fResultDirectory (""),
    fRunNumber (-1),
    fNEvents (0),
    fACFLock (toolbox::BSem::FULL, true), // the second argument is the recursive flag
    fServerPort (9090),
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
    fDataSender = new DataSender (this->getApplicationLogger() );

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
    this->getApplicationInfoSpace()->fireItemAvailable ("FedID", &fFedID);
    this->getApplicationInfoSpace()->fireItemAvailable ("HWDescriptionFile", &fHWDescriptionFile);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataDirectory", &fDataDirectory);
    this->getApplicationInfoSpace()->fireItemAvailable ("ResultDirectory", &fResultDirectory);
    this->getApplicationInfoSpace()->fireItemAvailable ("runNumber", &fRunNumber);
    this->getApplicationInfoSpace()->fireItemAvailable ("NEvents", &fNEvents);
    this->getApplicationInfoSpace()->fireItemAvailable ("WriteRAW", &fRAWFile);
    this->getApplicationInfoSpace()->fireItemAvailable ("WriteDAQ", &fDAQFile);
    this->getApplicationInfoSpace()->fireItemAvailable ("ShortPause", &fShortPause);
    this->getApplicationInfoSpace()->fireItemAvailable ("THttpServerPort", &fServerPort);

    //Data to EVM
    this->getApplicationInfoSpace()->fireItemAvailable ("SendData", &fSendData);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataDestination", &fDataDestination);
    this->getApplicationInfoSpace()->fireItemAvailable ("DQMPostScale", &fDQMPostScale);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataSourceHost", &fSourceHost);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataSourcePort", &fSourcePort);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataSinkHost", &fSinkHost);
    this->getApplicationInfoSpace()->fireItemAvailable ("DataSinkPort", &fSinkPort);

    //Playback Mode
    this->getApplicationInfoSpace()->fireItemAvailable ("PlaybackMode", &fPlaybackMode);
    this->getApplicationInfoSpace()->fireItemAvailable ("PlaybackFile", &fPlaybackFile);

    // Bind SOAP callbacks for control messages
    xoap::bind (this, &DTCSupervisor::fireEvent, "Initialise", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Configure", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Enable", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Halt", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Stop", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Pause", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Resume", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Destroy", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "SetValues", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "GetValues", XDAQ_NS_URI);

#ifdef __OTSDAQ__
    //need to bind these in addition if we are dealing with OTSDAQ environment
    xoap::bind (this, &DTCSupervisor::fireEvent, "Initialize", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Start", XDAQ_NS_URI);
    xoap::bind (this, &DTCSupervisor::fireEvent, "Shutdown", XDAQ_NS_URI);
#endif

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
        fGUI->fFedID = &fFedID;
        fGUI->fHWDescriptionFile = &fHWDescriptionFile;
        fGUI->fDataDirectory = &fDataDirectory;
        fGUI->fResultDirectory = &fResultDirectory;
        fGUI->fRunNumber = &fRunNumber;
        fGUI->fNEvents = &fNEvents;
        fGUI->fEventCounter = &fEventCounter;

        fGUI->fRAWFile = &fRAWFile;
        fGUI->fDAQFile = &fDAQFile;

        fGUI->fSendData = &fSendData;
        fGUI->fDQMPostScale = &fDQMPostScale;
        fGUI->fDataDestination = &fDataDestination;
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
        ss << GREEN << "FedID: " << RED << fFedID.toString() << GREEN << " set!" << std::endl;
        ss <<  GREEN << "HW Description file: " << RED << fHWDescriptionFile.toString() << GREEN << " set!" << std::endl;
        ss << "Write RAW data: " << RED << fRAWFile.toString() << GREEN << " set!" << std::endl;
        ss << "Write DAQ data: " << RED << fDAQFile.toString() << GREEN << " set!" << std::endl;
        ss << "Send DAQ data: " << RED << fSendData.toString() << GREEN << " set!" << std::endl;

        if (fSendData)
        {
            if (fDataDestination.toString() == "DQM" || fDataDestination.toString() == "EVM" )
            {
                ss << "Data Destination: " << RED << fDataDestination.toString() << GREEN << " set!" << std::endl;

                if (fDataDestination.toString() == "DQM")
                    ss << "DQM Post Scale Factor: " << RED << fDQMPostScale.toString() << GREEN << " set!" << std::endl;
            }
            else
            {
                ss << BOLDRED << "Warning: Data Destination: " << fDataDestination.toString() <<  " not recognized - assuming EVM!" << GREEN << std::endl;
                fDataDestination = "EVM";
            }

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
        fDataSender->setDestination (fDataDestination.toString() );
        fDataSender->setPostScale (fDQMPostScale);
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
            cCalibration.Initialise (fGUI->fAllChannels, true);
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
            cPedeNoise.Initialise (fGUI->fAllChannels, true);
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
        fACFLock.take();
        BeBoard* cBoard = fSystemController->fBoardVector.at (0);
        uint32_t cEventCount = fSystemController->ReadData (cBoard, false );
        fEventCounter += cEventCount;

        if (cEventCount != 0 && (fDAQFile || fSendData) )
        {
            const std::vector<Event*>& events = fSystemController->GetEvents ( cBoard );
            fACFLock.give();
            std::vector<SLinkEvent> cSLinkEventVec;

            for (auto cEvent : events)
            {
                SLinkEvent cSLev =  cEvent->GetSLinkEvent (cBoard);

                if (fDAQFile)
                    fSLinkFileHandler->set (cSLev.getData<uint32_t>() );

                cSLinkEventVec.push_back (cSLev);
            }

            if (fSendData)
                fDataSender->enqueueEvent (cSLinkEventVec);

            //#ifdef __OTSDAQ__
            //uint32_t cExactTriggers = fSystemController->fBeBoardInterface->ReadBoardReg (cBoard, "fc7_daq_stat.fast_command_block.trigger_in_counter");
            //LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDRED << "Exact number of Triggers read from FW counter: " << BOLDBLUE << cExactTriggers << RESET);
            //#endif

        }

        else
            fACFLock.give();
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
        {
            //fSystemController->Stop(cBoard);
            fFSM.fireEvent ("Stop", this);
        }

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

        if (cDataVec.size() != 0 && cDataVec.size() % fPlaybackEventSize32 == 0) // there is still some data in the file
        {
            size_t cCalcEventSize = cDataVec.size() / fPlaybackEventSize32;

            fSystemController->setData (cBoard, cDataVec, cCalcEventSize);

            fEventCounter += cCalcEventSize;

            std::this_thread::sleep_for (std::chrono::milliseconds (fShortPause) );

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
            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDYELLOW << "Warning, read file chunk of  " << cDataVec.size() << " words - this is not a multiple of the event size: " << fPlaybackEventSize32 << " -stopping playback job!" << RESET);

            // this is true only once we have reached the end of the file
            // so we need to close the file handler
            fSystemController->closeFileHandler();

            //if (fFSM.getCurrentState() == 'E')
            //fFSM.fireEvent ("Stop", this);

            return false;
        }

        fACFLock.give();
    }
    else //  we are dealing with .daq data that we want to send
    {
        //read the uint64_t vector from ifstream (fPlaybackIfstream)
        std::vector<SLinkEvent> cSLinkEventVec = this->readSLinkFromFile (cNEvents);

        if (cSLinkEventVec.size() != 0)
        {
            fEventCounter += cSLinkEventVec.size();

            //for(auto cEv: cSLinkEventVec)
            //cEv.print(std::cout);

            // still some data in the file
            if (fSendData)
                fDataSender->enqueueEvent (cSLinkEventVec);

            return true;
        }
        else
        {
            // this is true only once we have reached the end of the file
            // therefore we need to close the filestream
            if (fPlaybackIfstream.is_open() )
                fPlaybackIfstream.close();

            //if (fFSM.getCurrentState() == 'E')
            //fFSM.fireEvent ("Stop", this);
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

        //the below is normally called from the GUI but with RCMS this is a problem, so rather call it here!
        //the easiest distinction is to check if the fHwXMLString is empty (this means that Initialize has not been triggered from the GUI

        std::ostringstream cLogStream;
        //now convert the HW Description HTMLString to an xml string for Initialize of Ph2ACF
        std::string cTmpFormString = cleanup_before_XSLT (fGUI->fHWFormString);
        fGUI->fHwXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (XMLSTYLESHEET), cLogStream, false);
        //now do the same for the Settings
        cTmpFormString = cleanup_before_XSLT_Settings (fGUI->fSettingsFormString);
        fGUI->fSettingsXMLString = XMLUtils::transformXmlDocument (cTmpFormString, expandEnvironmentVariables (SETTINGSSTYLESHEETINVERSE), cLogStream, false);

        if (cLogStream.tellp() > 0) LOG4CPLUS_INFO (this->getApplicationLogger(), cLogStream.str() );

        if (fGUI->fHwXMLString.empty() ) LOG4CPLUS_ERROR (this->getApplicationLogger(), RED << "Error, HWXML STring empty for whatever reason!" << RESET);

        //expand all file paths from HW Description xml string
        complete_file_paths (fGUI->fHwXMLString);

        //this is a temporary solution to keep the logs - in fact I should tail the logfile that I have to configure properly before
        std::ostringstream cPh2LogStream;

        fSystemController->InitializeSettings (fGUI->fSettingsXMLString, cPh2LogStream, false);
        fSystemController->InitializeHw (fGUI->fHwXMLString, cPh2LogStream, false);
        fACFLock.give();

        LOG (INFO) << cPh2LogStream.str() ;
        this->updateLogs();

        //this needs to happen in initializing so I can be sure that any GUI changes have effect
        if (fSendData)
        {
            //set the source and sink for the TCP data sender
            fDataSender->setDestination (fDataDestination.toString() );
            fDataSender->setPostScale (fDQMPostScale);
            fDataSender->setSource (fSourceHost.toString(), fSourcePort);
            fDataSender->setSink (fSinkHost.toString(), fSinkPort);
            fGUI->fDataSenderTable = fDataSender->generateStatTable();
        }
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

#ifndef __OTSDAQ__
                    //if we are NOT in the OTSDAQ environment, we get the run number at configure!
                    int cRunNumber = fRunNumber;

                    if (fRunNumber == -1) fGetRunnumberFromFile = true;

                    //get the runnumber from the storage file in Ph2_ACF
                    if (fGetRunnumberFromFile) // this means we want the run number from the file
                    {
                        getRunNumber (expandEnvironmentVariables ("${PH2ACF_ROOT}"), cRunNumber, true) ;
                        fRunNumber = cRunNumber;
                    }

                    std::string cRawOutputFile = fDataDirectory.toString() + string_format ("run_%05d.raw", cRunNumber);

                    //first add a filehandler for the raw data to SystemController
                    if (fRAWFile)
                    {
                        bool cExists = Ph2TkDAQ::mkdir (fDataDirectory.toString() );

                        if (!cExists)
                            LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Data Directory : " << BOLDBLUE << fDataDirectory.toString() << BOLDGREEN << " does not exist - creating!" << RESET);

                        fSystemController->addFileHandler ( cRawOutputFile, 'w' );
                        fSystemController->initializeFileHandler();
                        cRawOutputFile.insert (cRawOutputFile.find (".raw"), "_fedId");
                        LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Saving raw data to: " << BOLDBLUE << cRawOutputFile << RESET);
                    }

#endif

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
                fGetRunnumberFromFile = true;

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
            fDataSender->openConnection();
            fGUI->fDataSenderTable = fDataSender->generateStatTable();
        }

#ifndef __OTSDAQ__

        //if we are NOT in the OTSDAQ environment, we do this now, otherwise on start
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

#endif

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

                //fGUI->fDataSenderTable = fDataSender->generateStatTable();
            }
        }

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

#ifdef __OTSDAQ__
                //OTSDAQ sends the run number on Start (not enable) so we can only create the file handler here
                int cRunNumber = fRunNumber;

                if (fRunNumber == -1) fGetRunnumberFromFile = true;

                //get the runnumber from the storage file in Ph2_ACF
                if (fGetRunnumberFromFile) // this means we want the run number from the file
                {
                    getRunNumber (expandEnvironmentVariables ("${PH2ACF_ROOT}"), cRunNumber, true) ;
                    fRunNumber = cRunNumber;
                }

                std::string cRawOutputFile = fDataDirectory.toString() + string_format ("run_%05d.raw", cRunNumber);

                //first add a filehandler for the raw data to SystemController
                if (fRAWFile)
                {
                    bool cExists = Ph2TkDAQ::mkdir (fDataDirectory.toString() );

                    if (!cExists)
                        LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Data Directory : " << BOLDBLUE << fDataDirectory.toString() << BOLDGREEN << " does not exist - creating!" << RESET);


                    fACFLock.take();
                    fSystemController->addFileHandler ( cRawOutputFile, 'w' );
                    fSystemController->initializeFileHandler();
                    fACFLock.give();

                    cRawOutputFile.insert (cRawOutputFile.find (".raw"), "_fedId");
                    LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDGREEN << "Saving raw data to: " << BOLDBLUE << cRawOutputFile << RESET);
                }

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

#endif
                //here, read the CBC files for Sarah
                fACFLock.take();

                for (auto cBoard : fSystemController->fBoardVector)
                    for (auto cFe : cBoard->fModuleVector)
                        for (auto cCbc : cFe->fCbcVector)
                            fSystemController->fCbcInterface->ReadCbc (cCbc);

                fACFLock.give();
                this->dumpCbcFiles (fResultDirectory, "enabling");

                //then, make sure that the event counter is 0 since we are just starting the run
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

            fDataSender->closeConnection();
            fGUI->fDataSenderTable = fDataSender->generateStatTable();
        }

        if (fRAWFile)
        {
            //still need to destroy the file handler for raw data taking since we'll initialize a new one when re-configuring
            fACFLock.take();
            fSystemController->closeFileHandler();
            fACFLock.give();
        }

        if (fDAQFile)
        {
            fSLinkFileHandler->closeFile();

            if (fSLinkFileHandler) delete fSLinkFileHandler;

            fSLinkFileHandler = nullptr;
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
        //figure out if we were supposed to run any calibrations
        int cEnabledProcedures = 0;
        bool cDataTaking = false;

        for (auto cProcedure : fGUI->fProcedureMap)
        {

            if (cProcedure.second && cProcedure.first != "Data Taking") cEnabledProcedures++;

            if (cProcedure.second && cProcedure.first == "Data Taking") cDataTaking = true;

            if (cProcedure.second)
                LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDYELLOW << "Stopping " << BOLDBLUE << cProcedure.first << RESET);
        }

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

        if (cDataTaking && cEnabledProcedures == 0)
        {
            //here, read the CBC files for Sarah
            fACFLock.take();

            for (auto cBoard : fSystemController->fBoardVector)
                for (auto cFe : cBoard->fModuleVector)
                    for (auto cCbc : cFe->fCbcVector)
                        fSystemController->fCbcInterface->ReadCbc (cCbc);

            fACFLock.give();
            this->dumpCbcFiles (fResultDirectory, "stopping");
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

xoap::MessageReference DTCSupervisor::fireEvent (xoap::MessageReference msg) throw (xoap::exception::Exception)
{
    xoap::SOAPPart part = msg->getSOAPPart();
    xoap::SOAPEnvelope env = part.getEnvelope();
    xoap::SOAPBody body = env.getBody();
    DOMNode* node = body.getDOMNode();
    DOMNodeList* bodyList = node->getChildNodes();

    for (unsigned int i = 0; i < bodyList->getLength(); i++)
    {
        DOMNode* command = bodyList->item (i);

        if (command->getNodeType() == DOMNode::ELEMENT_NODE)
        {
            std::string commandName = xoap::XMLCh2String (command->getLocalName() );

            if (commandName == "Initialise" ||
                    commandName == "Configure" ||
                    commandName == "Enable" ||
                    commandName == "Stop" ||
                    commandName == "Pause" ||
                    commandName == "Resume" ||
                    commandName == "Halt" ||
#ifdef __OTSDAQ__
                    commandName == "Initialize" ||
                    commandName == "Shutdown" ||
                    commandName.find ("Start") != std::string::npos ||
#endif
                    commandName == "Destroy")
                try
                {
                    //toolbox::Event::Reference e (new toolbox::Event (commandName, this) );
                    //fsm_.fireEvent (e);

#ifdef __OTSDAQ__
                    //some OTSDAQ specific code! need to translate the OTSDDAQ commands to the standard FED FSM callbacks here!

                    if (commandName == "Initialize")
                        commandName = "Initialise";

                    if (commandName == "Shutdown")
                        commandName = "Destroy";

                    if (commandName.find ("Start") != std::string::npos)
                    {
                        commandName = "Enable";
                        //here also need to parse the run number from the message content
                        DOMNamedNodeMap*  attributes = command->getAttributes();

                        for (unsigned int i = 0; i < attributes->getLength(); i++)
                        {
                            if (xoap::XMLCh2String (attributes->item (i)->getNodeName() ) == "RunNumber")
                            {
                                int cRunNumber;
                                sscanf (xoap::XMLCh2String (attributes->item (i)->getNodeValue() ).c_str(), "%d", &cRunNumber);
                                fRunNumber = cRunNumber;
                                LOG4CPLUS_INFO (this->getApplicationLogger(), RED << "Received Run Number on Start Command from OTSDAQ " << fRunNumber.toString() << RESET);
                            }
                        }
                    }

#endif

                    fFSM.fireEvent (commandName, this);
                }
                catch (toolbox::fsm::exception::Exception& e)
                {
                    XCEPT_RETHROW (xcept::Exception, "invalid command", e);
                }

            if (commandName == "SetValues")
            {
                std::cout << "DTCSupervisor:: " << "I got a command " << std::endl;
                // All attributes
                DOMNamedNodeMap*  attributes = command->getAttributes();
                DOMNode*  e = attributes->getNamedItem (XMLString::transcode ("name") );

                if (e == NULL)
                {
                    std::cout << "No name tag" << std::endl;
                    continue;
                }

                std::cout << "DTCSupervisor:: " << "Name is " << xoap::XMLCh2String (e->getNodeValue() ) << std::endl;

                xdata::Serializable* s = getApplicationInfoSpace()->find (xoap::XMLCh2String (e->getNodeValue() ) );

                if (s != NULL)
                {
                    std::cout << "DTCSupervisor:: " << "Type is " << s->type();

                    if (s->type() == "int")
                    {
                        xdata::Integer* si = (xdata::Integer*) s;
                        int d;
                        DOMNode*  ev = attributes->getNamedItem (XMLString::transcode ("value") );
                        sscanf (xoap::XMLCh2String (ev->getNodeValue() ).c_str(), "%d", &d);
                        si->value_ = d;
                        std::cout << "DTCSupervisor:: " << "Type & Value:  " << si->type() << " " << *si << std::endl;
                    }

                    if (s->type() == "unsigned long")
                    {
                        xdata::UnsignedLong* si = (xdata::UnsignedLong*) s;
                        unsigned long d;
                        DOMNode*  ev = attributes->getNamedItem (XMLString::transcode ("value") );
                        sscanf (xoap::XMLCh2String (ev->getNodeValue() ).c_str(), "%ld", &d);
                        si->value_ = d;
                        std::cout << "DTCSupervisor:: " << "Type & Value:  " << si->type() << " " << *si << std::endl;
                    }

                    if (s->type() == "unsigned short")
                    {
                        xdata::UnsignedShort* si = (xdata::UnsignedShort*) s;
                        int d;
                        DOMNode*  ev = attributes->getNamedItem (XMLString::transcode ("value") );
                        sscanf (xoap::XMLCh2String (ev->getNodeValue() ).c_str(), "%d", &d);
                        si->value_ = d;
                        std::cout << "DTCSupervisor:: " << "Type & Value:  " << si->type() << " " << *si << std::endl;
                    }

                    if (s->type() == "unsigned int")
                    {
                        xdata::UnsignedInteger32* si = (xdata::UnsignedInteger32*) s;
                        int d;
                        DOMNode*  ev = attributes->getNamedItem (XMLString::transcode ("value") );
                        sscanf (xoap::XMLCh2String (ev->getNodeValue() ).c_str(), "%d", &d);
                        si->value_ = d;
                        std::cout << "DTCSupervisor:: " << "Type & Value:  " << si->type() << " " << *si << std::endl;
                    }

                    if (s->type() == "string")
                    {
                        xdata::String* si = (xdata::String*) s;
                        DOMNode*  ev = attributes->getNamedItem (XMLString::transcode ("value") );
                        si->value_ = xoap::XMLCh2String (ev->getNodeValue() ).c_str();
                        std::cout << "DTCSupervisor:: " << "Type & Value:  " << si->type() << " " << si->value_ << std::endl;
                    }

                    if (s->type() == "bool")
                    {
                        xdata::Boolean* si = (xdata::Boolean*) s;
                        DOMNode*  ev = attributes->getNamedItem (XMLString::transcode ("value") );

                        if (xoap::XMLCh2String (ev->getNodeValue() ) == "true")
                            si->value_ = true;
                        else
                            si->value_ = false;

                        std::cout << "DTCSupervisor:: " << "Type & Value:  " << si->type() << " " << *si << std::endl;
                    }

                }

            }

            xoap::MessageReference reply = xoap::createMessage();
            xoap::SOAPEnvelope envelope = reply->getSOAPPart().getEnvelope();

#ifdef __OTSDAQ__

            if (commandName == "Initialise") commandName = "Initialize";

            if (commandName == "Destroy") commandName = "Shutdown";

            if (commandName == "Enable") commandName = "Start";

#endif

            xoap::SOAPName responseName = envelope.createName ( commandName + "Response", "xdaq", XDAQ_NS_URI);
            envelope.getBody().addBodyElement ( responseName );
            return reply;
        }
    }

    XCEPT_RAISE (xcept::Exception, "command not found");
}

void DTCSupervisor::dumpCbcFiles (std::string pDirectoryName, std::string pTransition)
{
    // visitor to call dumpRegFile on each Cbc
    struct RegMapDumper : public HwDescriptionVisitor
    {
        std::string fDirectoryName;
        std::string fTransition;
        int fRunNumber;
        std::string fFileName;
        RegMapDumper ( std::string pDirectoryName, std::string pTransition, xdata::Integer pRunNumber ) : fDirectoryName ( pDirectoryName ), fTransition (pTransition), fRunNumber (pRunNumber) {};
        void visit ( Cbc& pCbc )
        {
            if ( !fDirectoryName.empty() )
            {
                TString cFilename = fDirectoryName + Form ( "/FE%dCBC%dRun%05d_%s.txt", pCbc.getFeId(), pCbc.getCbcId(), fRunNumber, fTransition.c_str() );
                fFileName = cFilename.Data();
                pCbc.saveRegMap (fFileName);
            }

            //else LOG4CPLUS_INFO(this->getApplicationLogger(), RED << "Error: no results Directory initialized! "<< RESET);
        }
    };

    RegMapDumper cDumper ( pDirectoryName, pTransition, fRunNumber);
    fACFLock.take();
    fSystemController->accept ( cDumper );
    fACFLock.give();
    std::string cFilename = cDumper.fFileName;

    LOG4CPLUS_INFO (this->getApplicationLogger(), BOLDBLUE << "Configfiles for all Cbcs written to " << pDirectoryName << " on Transition " << pTransition << " - Filename: " << cFilename << RESET);
}
