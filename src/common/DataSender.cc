#include "ACFSupervisor/DataSender.h"

using namespace Ph2TkDAQ;

DataSender::DataSender (log4cplus::Logger pLogger) :
    fDataDestination (""),
    fPostScaleFactor (1),
    fSourceHost (""),
    fSourcePort (0),
    fSinkHost (""),
    fSinkPort (0),
    fSocketOpen (false),
    fLogger (pLogger),
    fSockfd (0),
    fEventsProcessed (0),
    fEventFrequency (0)
{
    LOG4CPLUS_INFO (fLogger, "Creating DataSender");

}

DataSender::DataSender (std::string pDataDestination, std::string pSourceHost, uint32_t pSourcePort, std::string pSinkHost, uint32_t pSinkPort, log4cplus::Logger pLogger) :
    fDataDestination (pDataDestination),
    fPostScaleFactor (1),
    fSourceHost (pSourceHost),
    fSourcePort (pSourcePort),
    fSinkHost (pSinkHost),
    fSinkPort (pSinkPort),
    fSocketOpen (false),
    fLogger (pLogger),
    fSockfd (0),
    fEventsProcessed (0),
    fEventFrequency (0)
{
    LOG4CPLUS_INFO (fLogger, "Creating DataSender");

}

DataSender::~DataSender()
{
    LOG4CPLUS_INFO (fLogger, "Destroying DataSender");
}

std::vector<uint64_t> DataSender::generateFEROLHeader (uint16_t pBlockNumber, bool pFirst, bool pLast, uint16_t pNWords64, uint16_t pFEDId, uint32_t pL1AId)
{
    std::vector<uint64_t> cVec;

    uint64_t cFirstWord = 0;
    uint64_t cSecondWord = 0;

    cFirstWord |= ( (uint64_t) FEROLMAGICWORD & 0xFFFF) << 48 | ( (uint64_t) pBlockNumber & 0x7FF) << 32 | ( (uint64_t) pNWords64 & 0x3FF);

    if (pFirst) cFirstWord |= (uint64_t) 1 << 31;

    if (pLast) cFirstWord |= (uint64_t) 1 << 30;

    cSecondWord |= ( (uint64_t) pFEDId & 0xFFF) << 32 | ( (uint64_t) pL1AId & 0xFFFFF);

    // I need to byte-order reverse the FEROL header for the EVM to recognize it
    cVec.push_back (__builtin_bswap64 (cFirstWord) );
    cVec.push_back (__builtin_bswap64 (cSecondWord) );

    return cVec;
}

std::vector<uint64_t> DataSender::generateTCPPackets (SLinkEvent& pEvent)
{
    //std::vector<std::vector<uint64_t>> cBufVec;
    std::vector<uint64_t> cBufVec;

    const uint32_t ferolBlockSize64 = 512;//in Bytes
    const uint32_t ferolHeaderSize64 = 2;//in Bytes
    const uint32_t ferolPayloadSize64 = ferolBlockSize64 - ferolHeaderSize64;

    //first, figure out the size of my slink event
    size_t cEventSize64  = pEvent.getSize64();
    uint16_t cSourceId = pEvent.getSourceId();
    uint32_t cL1AId = pEvent.getLV1Id();
    //instead of ceil
    uint32_t cNbBlock = (cEventSize64 + ferolPayloadSize64 - 1) / ferolPayloadSize64;
    //LOG4CPLUS_INFO(fLogger, "Size: " << cEventSize64 << " L1A ID: " << cL1AId << " Nb block: " << cNbBlock );


    //get the Event Data as vector of uint64_t
    std::vector<uint64_t> cEventData = pEvent.getData<uint64_t>();

    bool cFirst = true;
    bool cLast = false;
    uint32_t cBlockIdx = 0;

    while (!cLast)
    {
        if (cNbBlock == 1 && cBlockIdx == 0)
        {
            //one and only one block as vector of uint64_t
            cLast = true;
            //the BufVec is empty at this stage
            cBufVec = this->generateFEROLHeader (cBlockIdx, cFirst, cLast, cEventSize64, cSourceId, cL1AId);
            //insert event payload
            cBufVec.insert (cBufVec.end(), cEventData.begin(), cEventData.end() );
            //cBufVec.push_back (cTmpVec);
        }

        else if (cNbBlock > 1 && cBlockIdx == 0)
        {
            //first block of multi block event
            //the BufVec is empty at this stage
            cBufVec = this->generateFEROLHeader (cBlockIdx, cFirst, cLast, ferolPayloadSize64, cSourceId, cL1AId);
            //insert event payload
            cBufVec.insert (cBufVec.end(), cEventData.begin(), cEventData.begin() + ferolPayloadSize64);
            //cBufVec.push_back (cTmpVec);
        }
        else if (cBlockIdx > 0 && cBlockIdx < cNbBlock)
        {
            //a Block in the middle of a multi-block event
            cFirst = false;
            std::vector<uint64_t> cTmpVec = this->generateFEROLHeader (cBlockIdx, cFirst, cLast, ferolPayloadSize64, cSourceId, cL1AId);
            //insert the FEROL header in BufVec
            cBufVec.insert (cBufVec.end(), cTmpVec.begin(), cTmpVec.end() );
            //insert event payload
            cBufVec.insert (cBufVec.end(), cEventData.begin() + cBlockIdx * ferolPayloadSize64, cEventData.begin() + (cBlockIdx + 1) *ferolPayloadSize64);
            //cBufVec.push_back (cTmpVec);
        }
        else if (cBlockIdx > 0 && cBlockIdx + 1 == cNbBlock)
        {
            //the last block of an event
            cFirst = false;
            cLast = true;
            std::vector<uint64_t> cTmpVec = this->generateFEROLHeader (cBlockIdx, cFirst, cLast, cEventSize64 - (cBlockIdx * ferolPayloadSize64), cSourceId, cL1AId);
            //insert the FEROL header in BufVec
            cBufVec.insert (cBufVec.end(), cTmpVec.begin(), cTmpVec.end() );
            //insert event payload
            cBufVec.insert (cBufVec.end(), cEventData.begin() + cBlockIdx * ferolPayloadSize64, cEventData.end() );
            //cBufVec.push_back (cTmpVec);
        }

        cBlockIdx++;
    }


    return cBufVec;
}
std::vector<uint64_t> DataSender::generateUDPPackets (SLinkEvent& pEvent)
{
    return pEvent.getData<uint64_t>();
}

bool DataSender::sendData ()
{
    //first, dequeue event - this blocks for some period and then returns
    std::vector<SLinkEvent> cEventVec;
    bool cData = this->dequeueEvent (cEventVec);

    // if dequeue event returned true, we have a new SLinkEventVector that we need to process!
    if (cData)
    {
        //loop the events
        uint32_t cEventIndex = 0;

        for (auto& cEvent : cEventVec)
        {
            //if the data destination is the EVM, we just send every event over the TCP socket with the FEROL header and the whole shebang

            std::vector<uint64_t> cBufVec;

            //if the destination is th EVM, then do this for every event
            if (fDataDestination == "EVM")
                cBufVec = this->generateTCPPackets (cEvent);
            //if the destination is the DQM, make sure only every i-th event gets processed
            else if (fDataDestination == "DQM" && cEventIndex % fPostScaleFactor == 0)
                cBufVec = this->generateUDPPackets (cEvent);
            //if not every i-th event, continue but don't forget to increment the event index
            else
            {
                cEventIndex++;
                continue;
            }

            //this->print_databuffer (cBufVec, std::cout);

            //the we only get here in case we have decided to send

            //now write this Socket buffer vector at the socket
            ssize_t len = cBufVec.size() * sizeof (uint64_t);

            while ( len > 0 && fSocketOpen )
            {
                //consecutive storage of vector elements guaranteed by the standard, so this is possible
                //last argument is size of vector in bytes
                ssize_t written = 0;

                if (fDataDestination == "EVM") written = write (fSockfd, (char*) &cBufVec.at (0), cBufVec.size() * sizeof (uint64_t) );

                else if (fDataDestination == "DQM") written = sendto (fSockfd, (char*) &cBufVec.at (0), cBufVec.size() * sizeof (uint64_t), 0, (struct sockaddr*) &fsa_local, sizeof (fsa_local) );

                if ( written < 0 )
                {
                    if ( errno == EWOULDBLOCK )
                        std::this_thread::sleep_for (std::chrono::microseconds (100) );
                    else
                    {
                        std::ostringstream msg;
                        msg << "Failed to send data to " << fSinkHost << ":" << fSinkPort;
                        msg << " : " << strerror (errno);
                        XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
                    }

                }
                else if (written / sizeof (uint64_t) == cBufVec.size() )
                {
                    //LOG4CPLUS_INFO (fLogger, RED << "Wrote the whole buffer" << RESET);
                    len = 0;
                    cBufVec.clear();
                }
                else if (written / sizeof (uint64_t) != cBufVec.size() && written % sizeof (uint64_t) == 0)
                {
                    len -= written;
                    ssize_t written64 = written / sizeof (uint64_t);
                    //erase the written / sizeof() first words from the buffer vector, this should never happen though
                    cBufVec.erase (cBufVec.begin(), cBufVec.begin() + written64);
                    LOG4CPLUS_ERROR (fLogger, RED << "Error, did not write the whole buffer vector but only " << written64 << " words" << RESET);
                }
                else
                    LOG4CPLUS_ERROR (fLogger, RED << "Error, did only write " << written << " bytes" << RESET);
            }

            //better safe than sorry
            fCounterMutex.lock();
            fEventsProcessed++;
            fCounterMutex.unlock();
            cEventIndex++;
        }

        //better safe than sorry
        //this is to compute the frequency
        fCounterMutex.lock();
        double cElapsedTime = fTimer.getCurrentTime();

        if (cElapsedTime != 0)
            fEventFrequency = fEventsProcessed / fTimer.getCurrentTime();
        else
        {
            LOG4CPLUS_INFO (fLogger, RED << "Error, timer returned 0" << RESET);
            fEventFrequency = -999;
        }

        fCounterMutex.unlock();
    }

    //this might need to change when we run out of data
    return true;
}

void DataSender::openConnection()
{
    char myname[100];

    if (gethostname (myname, 100) != 0)
    {
        std::cout << "DS:: Failed to get hostname" << ": " << strerror (errno) << std::endl;
        XCEPT_RAISE (xdaq::exception::Exception, "DS:: Failed to get hostname");
    }

    if (std::string (myname) != fSourceHost)
        fSourceHost = std::string (myname);

    // if the destination is the EVM we use a TCP socket according to the config specified by CDAQ
    if (fDataDestination == "EVM")
    {
        fSockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
        fSocketOpen = true;

        if ( fSockfd < 0 )
        {
            XCEPT_RAISE (xdaq::exception::Exception, "Failed to open socket");
            fSocketOpen = false;
        }

        // Allow socket to reuse the address
        int yes = 1;

        if ( setsockopt (fSockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (yes) ) < 0 )
        {
            closeConnection();
            fSocketOpen = false;
            std::ostringstream msg;
            msg << "Failed to set SO_REUSEADDR on socket: " << strerror (errno);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }

        // Allow socket to reuse the port
        if ( setsockopt (fSockfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof (yes) ) < 0 )
        {
            closeConnection();
            fSocketOpen = false;
            std::ostringstream msg;
            msg << "Failed to set SO_REUSEPORT on socket: " << strerror (errno);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }

        // Set connection Abort on close
        struct linger so_linger;
        so_linger.l_onoff = 1;
        so_linger.l_linger = 0;

        if ( setsockopt (fSockfd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof (so_linger) ) < 0 )
        {
            closeConnection();
            fSocketOpen = false;
            std::ostringstream msg;
            msg << "Failed to set SO_LINGER on socket: " << strerror (errno);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }

        sockaddr_in sa_local;
        memset (&sa_local, 0, sizeof (sa_local) );
        sa_local.sin_family = AF_INET;
        sa_local.sin_port = htons (fSourcePort);
        sa_local.sin_addr.s_addr = inet_addr (fSourceHost.c_str() );

        if ( bind (fSockfd, (struct sockaddr*) &sa_local, sizeof (struct sockaddr) ) < 0 )
        {
            closeConnection();
            fSocketOpen = false;
            std::ostringstream msg;
            msg << "Failed to bind socket to local port " << fSourceHost << ":" << fSourcePort;
            msg << " : " << strerror (errno);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }

        addrinfo hints, *servinfo;
        memset (&hints, 0, sizeof (hints) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        char str[8];
        sprintf (str, "%d", fSinkPort);
        const int result = getaddrinfo (
                               fSinkHost.c_str(),
                               (char*) &str,
                               &hints, &servinfo);

        if ( result != 0 )
        {
            std::ostringstream msg;
            msg << "Failed to get server info for " << fSinkHost << ":" << fSinkPort;
            msg << " : " << gai_strerror (result);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }

        if ( connect (fSockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0 )
        {
            closeConnection();
            fSocketOpen = false;
            std::ostringstream msg;
            msg << "Failed to connect to " << fSinkHost << ":" << fSinkPort;
            msg << " : " << strerror (errno);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }

        // Set socket to non-blocking
        const int flags = fcntl (fSockfd, F_GETFL, 0);

        if ( fcntl (fSockfd, F_SETFL, flags | O_NONBLOCK) < 0 )
        {
            closeConnection();
            fSocketOpen = false;
            std::ostringstream msg;
            msg << "Failed to set O_NONBLOCK on socket: " << strerror (errno);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }
    }

    // if the destination is the DQM we use a plain UDP socket
    else if (fDataDestination == "DQM")
    {
        addrinfo hints, *servinfo, *p;
        memset (&hints, 0, sizeof (hints) );
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        char str[8];
        sprintf (str, "%d", fSourcePort);
        const int result = getaddrinfo (
                               fSourceHost.c_str(),
                               (char*) &str,
                               &hints, &servinfo);

        if ( result != 0 )
        {
            std::ostringstream msg;
            msg << "Failed to get server info for " << fSourceHost << ":" << fSourcePort;
            msg << " : " << gai_strerror (result);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }

        //loop through the results and bind to the first best
        for (p = servinfo; p != nullptr; p = p->ai_next)
        {
            fSockfd = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
            fSocketOpen = true;

            if ( fSockfd < 0 )
            {
                XCEPT_RAISE (xdaq::exception::Exception, "Failed to open socket");
                fSocketOpen = false;
                continue;
            }

            if ( bind (fSockfd, p->ai_addr, p->ai_addrlen) < 0 )
            {
                closeConnection();
                fSocketOpen = false;
                std::ostringstream msg;
                msg << "Failed to bind socket to port " << p->ai_addr;
                msg << " : " << strerror (errno);
                XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
                continue;
            }

            break;
        }

        if (p == nullptr)
        {
            fSocketOpen = false;
            std::ostringstream msg;
            msg << "Failed to bind socket";
            msg << " : " << strerror (errno);
            XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
        }

        //sockaddr_in fsa_local;
        fsa_local.sin_family = AF_INET;
        fsa_local.sin_port = htons (fSinkPort);

        if (inet_aton ("127.0.0.1", &fsa_local.sin_addr) == 0)
        {
            std::cout << "FATAL: Invalid IP address " << std::endl;
            exit (0);
        }

        //fsa_local.sin_addr.s_addr = inet_addr (fSinkHost.c_str() );
        memset (& (fsa_local.sin_zero), 0, sizeof (fsa_local) );
    }
}

void DataSender::closeConnection()
{
    if ( close (fSockfd) < 0 )
    {
        std::ostringstream msg;
        msg << "Failed to close socket to " << fSinkHost << ":" << fSinkPort;
        msg << " : " << strerror (errno);
        XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
    }

    fSockfd = 0;
    fSocketOpen = false;
}


void DataSender::enqueueEvent (std::vector<SLinkEvent> pEventVector)
{
    std::lock_guard<std::mutex> cLock (fMutex);
    fQueue.push (pEventVector);
    fEventSet.notify_one();
}

bool DataSender::dequeueEvent (std::vector<SLinkEvent>& pEventVector)
{
    std::unique_lock<std::mutex> cLock (fMutex);

    //here we wait for the condition variable to signal that we have data
    //if we get this signal, we dequeue a vector of events
    //if we don't receive the signal after 100 microseconds, we time out and go into the next iteration of this workloop
    bool cQueueEmpty = fEventSet.wait_for (cLock, std::chrono::microseconds (100), [&] { return  DataSender::fQueue.empty();});

    if (!cQueueEmpty)
    {
        pEventVector = fQueue.front();
        fQueue.pop();
    }

    return !cQueueEmpty;
}

void DataSender::print_databuffer (std::vector<uint64_t>& pBufVec, std::ostream& os)
{
    //this needs some work
    int cCounter = 0;

    for (auto& cWord : pBufVec)
    {
        os << BOLDBLUE << "#" <<  std::setw (3) << cCounter << RESET <<  " " << std::bitset<64> (cWord) << " " << BOLDBLUE <<  std::setw (16) << std::hex << cWord << std::dec << RESET << std::endl;
        cCounter++;
    }
}

uint64_t DataSender::getNEventsProcessed()
{
    std::lock_guard<std::mutex> cLock (fCounterMutex);
    return fEventsProcessed;
}

double DataSender::getEventFrequency()
{
    std::lock_guard<std::mutex> cLock (fCounterMutex);
    return fEventFrequency;
}

void DataSender::StartTimer()
{
    this->fTimer.start();
}
void DataSender::StopTimer()
{
    this->fTimer.stop();
}
void DataSender::ResetTimer()
{
    this->fTimer.reset();
}
cgicc::table DataSender::generateStatTable()
{
    cgicc::table cTable;
    cTable.set ("style", "margin-top: 20px; margin-left: 30px; font-size: 15pt;");

    cTable.add (cgicc::tr().add (cgicc::td ("Data Destination") ).add (cgicc::td (fDataDestination ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Post Scale Factor") ).add (cgicc::td (std::to_string (fPostScaleFactor ) ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Source Host") ).add (cgicc::td (fSourceHost ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Source Port") ).add (cgicc::td (std::to_string (fSourcePort) ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Destination Host") ).add (cgicc::td (fSinkHost ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Destination Port") ).add (cgicc::td (std::to_string (fSinkPort) ) ) );
    //fCounterMutex.lock();
    cTable.add (cgicc::tr().add (cgicc::td ("Processed Events") ).add (cgicc::td (std::to_string (this->getNEventsProcessed() ) ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Event processing Frequency") ).add (cgicc::td (std::to_string (this->getEventFrequency() ) ) ) );
    //fCounterMutex.unlock();

    return cTable;
}
