#ifndef __SLINKI2OSender_h__
#define __SLINKI2OSender_h__

#include "DTCSupervisor/I2OMsg.h"

#include "xdaq/Application.h"
#include "toolbox/mem/MemoryPoolFactory.h"
#include "toolbox/mem/CommittedHeapAllocator.h"
#include "i2o/utils/AddressMap.h"

class SLinkI2OSender : public I2OMsg
{
  public:
    SLinkI2OSender (xdaq::Application* pApp, int pTrailingSize, toolbox::mem::Pool* pMemoryPool, int pAllocate);
    SLinkI2OSender (xdaq::Application* pApp, int pTrailingSize = 0, int pMemoryPoolSize = 0, int pAllocate = 1);
    ~SLinkI2OSender();

    static toolbox::mem::Pool* CreateDefaultMemoryPool (xdaq::Application* pApp, int pTrailingSize, int pNbBlocks = 100);

    void setDestinationDescriptor (xdaq::ApplicationDescriptor* pDescriptor);
    void allocate (int pTrailingSize = 0);

    void send();

  private:
    static std::string getDefaultMemoryPoolName();
    xdaq::Application* fApp;
    xdaq::ApplicationDescriptor* fApplicationDescriptor;
    toolbox::mem::Pool* fPool;
};

std::string SLinkI2OSenderGetDefaultMemoryPoolName (xdaq::Application* pApp);
toolbox::mem::Pool* SLinkI2OSenderCreateDefaultMemoryPool (xdaq::Application* pApp, int pMemoryPoolSize);

#endif
