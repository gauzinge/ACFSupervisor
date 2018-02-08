/* Copyright 2017 Imperial Collge London
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Programmer :     Georg Auzinger
   Version :        1.0
   Date of creation:01/09/2017
   Support :        mail to : georg.auzinger@SPAMNOT.cern.ch
   FileName :       DataSender.h
*/

#ifndef _DATASENDER_H__
#define _DATASENDER_H__

//review these
#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/task/Action.h"
#include "toolbox/BSem.h"
#include <exception>
#include "xdaq/exception/Exception.h"
#include "xcept/Exception.h"
#include "toolbox/Runtime.h"
// ferol header but why?
#include "interface/shared/ferol_header.h"
//for logging
#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"
//to produce stat table
#include "cgicc/CgiDefs.h"
#include "cgicc/Cgicc.h"
#include "cgicc/HTTPHTMLHeader.h"
#include "cgicc/HTMLClasses.h"

//for the sockets
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <netdb.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

//for the queue
#include <mutex>
#include <queue>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <endian.h>

//from Ph2_ACF
#include "Utils/SLinkEvent.h"
#include "Utils/Timer.h"

//generic
#include <vector>
#include <iostream>

namespace Ph2TkDAQ {

    class DataSender
    {
#define FEROLMAGICWORD 0x475A

      public:
        DataSender (log4cplus::Logger pLogger);
        DataSender (std::string pDataDestination, std::string pSourceHost, uint32_t pSourcePort, std::string pSinkHost, uint32_t pSinkPort, log4cplus::Logger pLogger);
        ~DataSender();

        void setDestination (std::string pDataDestination)
        {
            fDataDestination = pDataDestination;
        }
        void setPostScale (int pPostScaleFactor)
        {
            fPostScaleFactor = pPostScaleFactor;
        }

        void setSource (std::string pSourceHost, uint32_t pSourcePort)
        {
            fSourceHost = pSourceHost;
            fSourcePort = pSourcePort;
        }
        void setSink (std::string pSinkHost, uint32_t pSinkPort)
        {
            fSinkHost = pSinkHost;
            fSinkPort = pSinkPort;
        }

        //to be called in ACFSupervisor::Configure
        void openConnection();
        //to be called in ACFSupervisor::Halt
        void closeConnection();
        //to be called in the workloop in the main ACF Supervisor thread
        bool sendData ();

        //to be called in the main DAQ workloop when the data is there
        void enqueueEvent (std::vector<SLinkEvent> pEventVector);
        //to be called inside SendData
        bool dequeueEvent (std::vector<SLinkEvent>& pEventVector);

        //get the number of processed events
        uint64_t getNEventsProcessed();
        double getEventFrequency();
        void StartTimer();
        void StopTimer();
        void ResetTimer();
        cgicc::table generateStatTable();

      private:
        std::vector<uint64_t> generateTCPPackets (SLinkEvent& pEvent);
        std::vector<uint64_t> generateUDPPackets (SLinkEvent& pEvent);
        std::vector<uint64_t> generateFEROLHeader (uint16_t pBlockNumber, bool pFirst, bool pLast, uint16_t pNWords64, uint16_t pFEDId, uint32_t pL1AId);
        void print_databuffer (std::vector<uint64_t>& pBufVec, std::ostream& os);

      public:
        std::string fDataDestination;
        int fPostScaleFactor;
        std::string fSourceHost;
        uint32_t fSourcePort;
        std::string fSinkHost;
        uint32_t fSinkPort;

      private:
        //SLinkEvent fEvent; [>!<SLinkEvent object to operate on after the dequeue operation <]
        //std::vector<std::vector<uint64_t>> fBufferVec; //not sure if this is needed - I am going to memcopy anyway?!
        mutable std::mutex fMutex;/*!< Mutex */
        mutable std::mutex fCounterMutex;/*!< Mutex */
        std::queue<std::vector<SLinkEvent>> fQueue; /*!<Queue to populate from set() and depopulate in writeFile() */
        std::condition_variable fEventSet;/*!< condition variable to notify writer thread of new data*/
        bool fSocketOpen;
        log4cplus::Logger fLogger;
        int fSockfd;
        uint64_t fEventsProcessed;
        Timer fTimer;
        double fEventFrequency;
        sockaddr_in fsa_local; //used for the UDP socket
    };
}
#endif
