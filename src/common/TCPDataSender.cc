#include "DTCSupervisor/TCPDataSender.h"

using namespace Ph2TkDAQ;

TCPDataSender::TCPDataSender (log4cplus::Logger pLogger) :
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
    LOG4CPLUS_INFO (fLogger, "Creating TCPDataSender");

}

TCPDataSender::TCPDataSender (std::string pSourceHost, uint32_t pSourcePort, std::string pSinkHost, uint32_t pSinkPort, log4cplus::Logger pLogger) :
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
    LOG4CPLUS_INFO (fLogger, "Creating TCPDataSender");

}

TCPDataSender::~TCPDataSender()
{
    LOG4CPLUS_INFO (fLogger, "Destroying TCPDataSender");
}

std::vector<uint64_t> TCPDataSender::generateFEROLHeader (uint16_t pBlockNumber, bool pFirst, bool pLast, uint16_t pNWords64, uint16_t pFEDId, uint32_t pL1AId)
{
    std::vector<uint64_t> cVec;

    uint64_t cFirstWord = 0;
    uint64_t cSecondWord = 0;

    cFirstWord |= ( (uint64_t) FEROLMAGICWORD & 0xFFFF) << 48 | ( (uint64_t) pBlockNumber & 0x7FF) << 32 | ( (uint64_t) pNWords64 & 0x3FF);

    if (pFirst) cFirstWord |= (uint64_t) 1 << 31;

    if (pLast) cFirstWord |= (uint64_t) 1 << 30;

    cSecondWord |= ( (uint64_t) pFEDId & 0xFFF) << 32 | ( (uint64_t) pL1AId & 0xFFFFF);
    cVec.push_back (cFirstWord);
    cVec.push_back (cSecondWord);

    return cVec;
}

std::vector<uint64_t> TCPDataSender::generateTCPPackets (SLinkEvent& pEvent)
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

bool TCPDataSender::sendData ()
{
    //first, dequeue event - this blocks for some period and then returns
    std::vector<SLinkEvent> cEventVec;
    bool cData = this->dequeueEvent (cEventVec);

    // if dequeue event returned true, we have a new SLinkEventVector that we need to process!
    if (cData)
    {
        std::vector<uint64_t> cSocketBufferVector;

        //loop the events
        for (auto& cEvent : cEventVec)
        {
            std::vector<uint64_t> cBufVec = this->generateTCPPackets (cEvent);

            this->print_databuffer (cBufVec, std::cout);

            //since there is a chance I need to write multiple events at once, why not concatenate the buffer vectors for all the events from the current iteration
            cSocketBufferVector.insert (cSocketBufferVector.end(), cBufVec.begin(), cBufVec.end() );

            //better safe than sorry
            fCounterMutex.lock();
            fEventsProcessed++;
            fCounterMutex.unlock();
        }

        //better safe than sorry
        fCounterMutex.lock();
        fEventFrequency = fEventsProcessed / fTimer.getCurrentTime();
        fCounterMutex.unlock();

        //now write this Socket buffer vector at the socket
        ssize_t len = cSocketBufferVector.size() * sizeof (uint64_t);

        while ( len > 0 && fSocketOpen )
        {
            //consecutive storage of vector elements guaranteed by the standard, so this is possible
            //last argument is size of vector in bytes
            const ssize_t written = write (fSockfd, (char*) &cSocketBufferVector.at (0), cSocketBufferVector.size() * sizeof (uint64_t) );

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
            else if (written % sizeof (uint64_t) == 0)
            {
                len -= written;
                ssize_t written64 = written / sizeof (uint64_t);
                //erase the written / sizeof() first words from the buffer vector, this should never happen though
                cSocketBufferVector.erase (cSocketBufferVector.begin(), cSocketBufferVector.begin() + written64);
                LOG4CPLUS_ERROR (fLogger, RED << "Error, did not write the whole buffer vector but only " << written64 << " words" << RESET);
            }
            else
                LOG4CPLUS_ERROR (fLogger, RED << "Error, did only write " << written << " bytes" << RESET);
        }

    }

    //this might need to change when we run out of data
    return true;
}

void TCPDataSender::openConnection()
{
    char myname[100];

    if (gethostname (myname, 100) != 0)
    {
        std::cout << "DS:: Failed to get hostname" << ": " << strerror (errno) << std::endl;
        XCEPT_RAISE (xdaq::exception::Exception, "DS:: Failed to get hostname");
    }

    if (std::string (myname) != fSourceHost)
        fSourceHost = std::string (myname);

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

void TCPDataSender::closeConnection()
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


void TCPDataSender::enqueueEvent (std::vector<SLinkEvent> pEventVector)
{
    std::lock_guard<std::mutex> cLock (fMutex);
    fQueue.push (pEventVector);
    fEventSet.notify_one();
}

bool TCPDataSender::dequeueEvent (std::vector<SLinkEvent>& pEventVector)
{
    std::unique_lock<std::mutex> cLock (fMutex);

    //here we wait for the condition variable to signal that we have data
    //if we get this signal, we dequeue a vector of events
    //if we don't receive the signal after 100 microseconds, we time out and go into the next iteration of this workloop
    bool cQueueEmpty = fEventSet.wait_for (cLock, std::chrono::microseconds (100), [&] { return  TCPDataSender::fQueue.empty();});

    if (!cQueueEmpty)
    {
        pEventVector = fQueue.front();
        fQueue.pop();
    }

    return !cQueueEmpty;
}

void TCPDataSender::print_databuffer (std::vector<uint64_t>& pBufVec, std::ostream& os)
{
    //this needs some work
    int cCounter = 0;

    for (auto& cWord : pBufVec)
    {
        os << BOLDBLUE << "#" <<  std::setw (3) << cCounter << RESET <<  " " << std::bitset<64> (cWord) << " " << BOLDBLUE <<  std::setw (16) << std::hex << cWord << std::dec << RESET << std::endl;
        cCounter++;
    }
}

uint64_t TCPDataSender::getNEventsProcessed()
{
    std::lock_guard<std::mutex> cLock (fCounterMutex);
    return fEventsProcessed;
}

double TCPDataSender::getEventFrequency()
{
    std::lock_guard<std::mutex> cLock (fCounterMutex);
    return fEventFrequency;
}

void TCPDataSender::StartTimer()
{
    this->fTimer.start();
}
void TCPDataSender::StopTimer()
{
    this->fTimer.stop();
}
void TCPDataSender::ResetTimer()
{
    this->fTimer.reset();
}
cgicc::table TCPDataSender::generateStatTable()
{
    cgicc::table cTable;
    cTable.set ("style", "margin-top: 20px; margin-left: 30px; font-size: 15pt;");

    cTable.add (cgicc::tr().add (cgicc::td ("Source Host") ).add (cgicc::td (fSourceHost ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Source Port") ).add (cgicc::td (std::to_string (fSourcePort) ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Destination Host") ).add (cgicc::td (fSinkHost ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Destination Port") ).add (cgicc::td (std::to_string (fSinkPort) ) ) );
    fCounterMutex.lock();
    cTable.add (cgicc::tr().add (cgicc::td ("Processed Events") ).add (cgicc::td (std::to_string (fEventsProcessed) ) ) );
    cTable.add (cgicc::tr().add (cgicc::td ("Event processing Frequency") ).add (cgicc::td (std::to_string (fEventFrequency) ) ) );
    fCounterMutex.unlock();

    return cTable;
}
