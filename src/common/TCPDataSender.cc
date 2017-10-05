#include "DTCSupervisor/TCPDataSender.h"

TCPDataSender::TCPDataSender (log4cplus::Logger pLogger) :
    fSourceHost (""),
    fSourcePort (0),
    fSinkHost (""),
    fSinkPort (0),
    fSocketOpen (false),
    fLogger (pLogger)
{
    LOG4CPLUS_INFO (fLogger, "Creating TCPDataSender");
}

TCPDataSender::TCPDataSender (std::string pSourceHost, uint32_t pSourcePort, std::string pSinkHost, uint32_t pSinkPortlog4cplus::Logger pLogger) :
    fSourceHost (pSourceHost),
    fSourcePort (pSourcePort),
    fSinkHost (pSinkHost),
    fSinkPort (pSinkPort),
    fSocketOpen (false),
    fLogger (pLogger)
{
    LOG4CPLUS_INFO (fLogger, "Creating TCPDataSender");
}

TCPDataSender::~TCPDataSender()
{
    LOG4CPLUS_INFO (fLogger, "Destroying TCPDataSender");
}

void TCPDataSender::openConnection()
{

}

void TCPDataSender::closeConnection()
{

}
