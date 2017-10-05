#ifndef _TCPDATASENDER_H__
#define _TCPDATASENDER_H__

//review these
#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/task/Action.h"
#include "toolbox/BSem.h"
#include "toolbox/Runtime.h"
// ferol header but why?
#include "interface/shared/ferol_header.h"
//for logging
#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"

//for the sockets
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>

//for the queue
#include <mutex>
#include <queue>
#include <atomic>
#include <thread>
#include <condition_variable>

//from Ph2_ACF
#include "Utils/SLinkEvent.h"

//generic
#include <vector>
#include <iostream>

namespace Ph2TkDAQ {

    class TCPDataSender
    {
#define FEROLMAGICWORD 0x475A

      public:
        TCPDataSender (log4plus::Logger pLogger);
        TCPDataSender (std::string pSourceHost, uint32_t pSourcePort, std::string pSinkHost, uint32_t pSinkPort, log4plus::Logger pLogger);
        ~TCPDataSender();

        //to be called in DTCSupervisor::Configure
        void openConnection();
        //to be called in DTCSupervisor::Halt
        void closeConnection();
        //to be called in the workloop in the main DTC Supervisor thread
        bool sendData (toolbox::task::WorkLoop* wl);

        //to be called in the main DAQ workloop when the data is there
        void enqueueEvent (SLinkEvent pEvent);
        //to be called inside SendData
        void dequeueEvent();

      private:
        void generateFEROLHeader();

      public:
        std::string fSourceHost;
        uint32_t fSourcePort;
        std::string fSinkHost;
        uint32_t fSinkPort;

      private:
        SLinkEvent fEvent; /*!<SLinkEvent object to operate on after the dequeue operation */
        uint64_t fBuffer[512]; //not sure if this is needed - I am going to memcopy anyway?!
        mutable std::mutex fMutex;/*!< Mutex */
        std::queue<SLinkEvent> fQueue; /*!<Queue to populate from set() and depopulate in writeFile() */
        std::condition_variable fEventSet;/*!< condition variable to notify writer thread of new data*/
        bool fSocketOpen;
        log4cplus::Logger fLogger;
    };
}
#endif
