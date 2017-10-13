#include "DTCSupervisor/TCPDataSender.h"

using namespace Ph2TkDAQ;

TCPDataSender::TCPDataSender (log4cplus::Logger pLogger) :
    fSourceHost (""),
    fSourcePort (0),
    fSinkHost (""),
    fSinkPort (0),
    fSocketOpen (false),
    fLogger (pLogger),
    fSockfd (0)
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
    fSockfd (0)

{
    LOG4CPLUS_INFO (fLogger, "Creating TCPDataSender");

}

TCPDataSender::~TCPDataSender()
{
    LOG4CPLUS_INFO (fLogger, "Destroying TCPDataSender");
}

std::vector<std::vector<uint64_t>> TCPDataSender::generateTCPPackets (SLinkEvent& pEvent)
{
    std::vector<std::vector<uint64_t>> cBufVec;

    const uint32_t ferolBlockSize64 = 512;//in Bytes
    const uint32_t ferolHeaderSize64 = 2;//in Bytes
    const uint32_t ferolPayloadSize64 = ferolBlockSize64 - ferolHeaderSize64;

    //first, figure out the size of my slink event
    size_t cEventSize64  = pEvent.getSize64();
    //instead of ceil
    uint32_t cNbBlock = (cEventSize64 + ferolPayloadSize64 - 1) / ferolPayloadSize64;
    LOG4CPLUS_INFO (fLogger, BOLDGREEN << "This event: lenght: " << cEventSize64 << " NBlock: (eventSize/" << ferolPayloadSize64 << "): " << cNbBlock << RESET);

    bool cFirst = true;
    bool cLast = false;
    uint32_t cBlockIdx = 0;

    while (!cLast)
    {
        if (cNbBlock == 1 && cBlockIdx == 0)
        {
            //one and only one block as vector of uint64_t
            cLast = true;
            std::vector<uint64_t> cTmpVec = this->generateFEROLHeader (pEvent, cBlockIdx, cFirst, cLast, cEventSize64, pEvent.getSourceId(), pEvent.getLV1Id() );

            for (auto cWord : cTmpVec)
                std::cout << std::hex << cWord << std::dec << std::endl;

            //cTmpVec.insert (cTmpVec.end(), pEvent.getData<uint64_t>().begin(), pEvent.getData<uint64_t>().end() );
            //cBufVec.push_back (cTmpVec);
        }

        //else if (cNbBlock > 1 && cBlockIdx == 0)
        //{
        ////first block of multi block event
        //std::vector<uint64_t> cTmpVec = this->generateFEROLHeader (pEvent, cBlockIdx, cFirst, cLast, ferolPayloadSize64, pEvent.getSourceId(), pEvent.getLV1Id() );
        //cTmpVec.insert (cTmpVec.end(), pEvent.getData<uint64_t>().begin(), pEvent.getData<uint64_t>().begin() + ferolPayloadSize64);
        //cBufVec.push_back (cTmpVec);
        //}
        //else if (cBlockIdx > 0 && cBlockIdx < cNbBlock)
        //{
        ////a Block in the middle of a multi-block event
        //cFirst = false;
        //std::vector<uint64_t> cTmpVec = this->generateFEROLHeader (pEvent, cBlockIdx, cFirst, cLast, ferolPayloadSize64, pEvent.getSourceId(), pEvent.getLV1Id() );
        //cTmpVec.insert (cTmpVec.end(), pEvent.getData<uint64_t>().begin() + cBlockIdx * ferolPayloadSize64, pEvent.getData<uint64_t>().begin() + (cBlockIdx + 1) *ferolPayloadSize64);
        //cBufVec.push_back (cTmpVec);
        //}
        //else if (cBlockIdx > 0 && cBlockIdx + 1 == cNbBlock)
        //{
        //cFirst = false;
        //cLast = true;
        //std::vector<uint64_t> cTmpVec = this->generateFEROLHeader (pEvent, cBlockIdx, cFirst, cLast, cEventSize64 - (cBlockIdx * ferolPayloadSize64), pEvent.getSourceId(), pEvent.getLV1Id() );
        //cTmpVec.insert (cTmpVec.end(), pEvent.getData<uint64_t>().begin() + cBlockIdx * ferolPayloadSize64, pEvent.getData<uint64_t>().end() );
        //cBufVec.push_back (cTmpVec);
        //}

        cBlockIdx++;
    }

    return cBufVec;
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
}

bool TCPDataSender::sendData ()
{
    std::cout << "Data sending workloop! " << std::endl;
    //first, dequeue event - this blocks for some period and then returns
    std::vector<SLinkEvent> cEventVec;
    bool cData = this->dequeueEvent (cEventVec);

    if (cData)
    {
        // if dequeue event returned true, we have a new SLinkEventVector that we need to process!
        for (auto& cEvent : cEventVec)
        {
            std::cout << cEvent << std::endl;
            std::vector<std::vector<uint64_t>> cBufVec = this->generateTCPPackets (cEvent);

            //for (auto& cBuff : cBufVec)
            //{
            //ssize_t len = cBuff.size() * sizeof (uint64_t); //in bytes?? //size of the buffer
            //char buf[4096];
            //memcpy (&buf, &cBuff[0], len);

            //for (int i = 0; i < cBuff.size(); i++)
            //{
            //std::cout << "DEBUG #" << i << " " << std::hex << cBuff.at (i) << " " << buf[i] << buf[i + 1] << buf[i + 2] << buf[i + 3] << buf[i + 4] << buf[i + 5] << buf[i + 6] << buf[i + 7] << std::dec << std::endl;
            //std::cout << std::endl;
            //}
            //}
        }
    }

    //if (this->dequeueEvent() )
    //{
    // if dequeue event returned true, we have a new SLinkEvent that we need to process!
    // this generates a vector of vectors fBufferVec that includes the FEROL headers
    //this->generateTCPPackets();

    //for (auto& cBuff : fBufferVec)
    //{
    //ssize_t len = cBuff.size() * sizeof (uint64_t); //in bytes?? //size of the buffer
    //char buf[4096];
    //memcpy (&buf, &cBuff[0], len);

    //for (int i = 0; i < cBuff.size(); i++)
    //{
    //std::cout << "DEBUG #" << i << " " << std::hex << cBuff.at (i) << " " << buf[i] << buf[i + 1] << buf[i + 2] << buf[i + 3] << buf[i + 4] << buf[i + 5] << buf[i + 6] << buf[i + 7] << std::dec << std::endl;
    //std::cout << std::endl;
    //}

    //TODO
    //while ( len > 0 && fSocketOpen )
    //{
    //const ssize_t written = write (fSockfd, buf, len);

    //if ( written < 0 )
    //{
    //if ( errno == EWOULDBLOCK )
    //::usleep (100);
    //else
    //{
    //std::ostringstream msg;
    //msg << "Failed to send data to " << fSinkHost << ":" << fSinkPort;
    //msg << " : " << strerror (errno);
    //XCEPT_RAISE (xdaq::exception::Exception, msg.str() );
    //}
    //}
    //else
    //{
    //len -= written;
    //*buf += written;
    //}
    //}
    //}
    //}

    return true;
}

std::vector<uint64_t> TCPDataSender::generateFEROLHeader (SLinkEvent& pEvent, uint16_t pBlockNumber, bool pFirst, bool pLast, uint16_t pNWords64, uint16_t pFEDId, uint32_t pL1AId)
{
    std::vector<uint64_t> cVec;

    if (pEvent.getSize64() != 0)
    {
        uint64_t cFirstWord = 0;
        uint64_t cSecondWord = 0;

        cFirstWord |= ( (uint64_t) FEROLMAGICWORD & 0xFFFF) << 48 | ( (uint64_t) pBlockNumber & 0x7FF) << 32 | ( (uint64_t) pNWords64 & 0x3FF);

        if (pFirst) cFirstWord |= (uint64_t) 1 << 31;

        if (pLast) cFirstWord |= (uint64_t) 1 << 30;

        cSecondWord |= ( (uint64_t) pFEDId & 0xFFF) << 32 | ( (uint64_t) pL1AId & 0xFFFFF);
        cVec.push_back (cFirstWord);
        cVec.push_back (cSecondWord);
    }
    else
        LOG4CPLUS_ERROR (fLogger, "Error, this SlinkEvent is empty!");

    return cVec;
}

void TCPDataSender::enqueueEvent (std::vector<SLinkEvent> pEventVector)
{
    std::lock_guard<std::mutex> cLock (fMutex);
    fQueue.push (pEventVector);
    fEventSet.notify_one();
}

bool TCPDataSender::dequeueEvent (std::vector<SLinkEvent>& pEventVector)
{
    //std::unique_lock<std::mutex> cLock (fMutex);

    //if (!fQueue.empty() )
    //{
    ////there is something in the queue still, event without the condition variable
    //fEvent.clear();
    //fEvent = fQueue.front();
    //fQueue.pop();
    //LOG4CPLUS_INFO (fLogger,  fEvent);
    //return true;
    //}
    //else
    //{
    //if (fEventSet.wait_for (cLock, std::chrono::milliseconds (500), [] {return true;}) )
    //{
    //std::cout << "received signal from condition variable" << std::endl;
    //fEvent.clear();
    //fEvent = fQueue.front();
    //fQueue.pop();
    //LOG4CPLUS_INFO (fLogger,  fEvent);
    //return true;
    //}
    //else
    //{
    //std::cout << "timed out after 500 ms!" << std::endl;
    //return false;
    //}
    //}

    //original implementation
    std::unique_lock<std::mutex> cLock (fMutex);

    while (fQueue.empty() )
        fEventSet.wait (cLock);

    pEventVector = fQueue.front();
    fQueue.pop();
}
