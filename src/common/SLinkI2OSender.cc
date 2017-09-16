#include "DTCSupervisor/SLinkI2OSender.h"

SLinkI2OSender::SLinkI2OSender (xdaq::Application* pApp, int pTrailingSize, toolbox::mem::Pool* pMemoryPool, int pAllocate) :
    fApp (pApp),
    fPool (pMemoryPool)
{
    if (pAllocate) allocate (pTrailingSize);
}

SLinkI2OSender::SLinkI2OSender (xdaq::Application* pApp, int pTrailingSize, int pMemoryPoolSize, int pAllocate)
{
    try
    {
        fApp = pApp;
        toolbox::net::URN cUrn ("toolbox-mem-pool", SLinkI2OSenderGetDefaultMemoryPoolName (fApp) );

        try
        {
            fPool = toolbox::mem::getMemoryPoolFactory()->findPool (cUrn);
        }

        catch (toolbox::mem::exception::MemoryPoolNotFound& e)
        {
            if (pMemoryPoolSize)
                fPool = CreateDefaultMemoryPool (fApp, pTrailingSize, pMemoryPoolSize);
            else
                fPool = CreateDefaultMemoryPool (fApp, pTrailingSize);
        }

        if (pAllocate)
            allocate (pTrailingSize);
    }

    catch (xcept::Exception& e)
    {
        std::cout << "Error in SLinkI2OSender constructor: " << e.what() << std::endl;
        throw;
    }
}

SLinkI2OSender::~SLinkI2OSender()
{

}

void SLinkI2OSender::setDestinationDescriptor (xdaq::ApplicationDescriptor* pDescriptor)
{
    try
    {
        fApplicationDescriptor = pDescriptor;
        this->setTargetAddress (i2o::utils::getAddressMap()->getTid (fApplicationDescriptor) );
    }
    catch (xcept::Exception& e)
    {
        std::cout << "Error setting the destination descriptor: " << e.what() << std::endl;
        throw;
    }
}

void SLinkI2OSender::allocate (int pTrailingSize)
{
    try
    {
        if (fPool)
        {
            int size = I2OMsg::getStandardHeaderSize() + pTrailingSize;
            toolbox::mem::Reference* cRef = toolbox::mem::getMemoryPoolFactory()->getFrame (fPool, size);
            I2OMsg::setReference (cRef, 0);
            I2OMsg::initializeDefaultValues();

            if (size % 4 != 0)
                XCEPT_RAISE (xcept::Exception, " size must be a multiple of 4 bytes  ");

            I2OMsg::setMessageSize (size >> 2);
            cRef->setDataSize (size);

            if ( ( (size >> 2) << 2) != size)
            {
                std::cout << size << " " << (size >> 2)
                          << std::endl << *this << std::endl;
                std::cout << pTrailingSize << std::endl;
            }

            this->setInitiatorAddress (i2o::utils::getAddressMap()->getTid (fApp->getApplicationDescriptor() ) );
        }
        else
        {
            ; // ??
        }
    }
    catch (xcept::Exception& e)
    {
        std::cout << "error during buffer allocation: " << e.what() << std::endl;
        throw;
    }
}

void SLinkI2OSender::send()
{
    try
    {
        fApp->getApplicationContext()->postFrame (I2OMsg::getReference(),
                fApp->getApplicationDescriptor(),
                fApplicationDescriptor);
    }
    catch (xcept::Exception& e)
    {
        std::cout << "error during send: " << e.what() << std::endl;
        throw;
    }
}

toolbox::mem::Pool* SLinkI2OSender::CreateDefaultMemoryPool (xdaq::Application* pApp, int pTrailingSize, int pNbBlocks)
{
    I2OMsg cMsg;
    return I2OSenderCreateDefaultMemoryPool (pApp, (pTrailingSize + cMSg.getEventDataBlockHeaderSize() ) * pNbBlocks);
}

std::string SLinkI2OSenderGetDefaultMemoryPoolName (xdaq::Application* pApp)
{
    std::ostringstream str;
    str << "memory pool for xdaq::Application = 0x" << pApp ;
    return str.str();
}

toolbox::mem::Pool* SLinkI2OSenderCreateDefaultMemoryPool (xdaq::Application* pApp, int pMemoryPoolSize)
{
    try
    {
        toolbox::mem::CommittedHeapAllocator* a = new toolbox::mem::CommittedHeapAllocator (pMemoryPoolSize);
        toolbox::net::URN urn ("toolbox-mem-pool", SLinkI2OSenderGetDefaultMemoryPoolName (pApp) );
        return toolbox::mem::getMemoryPoolFactory()->createPool (urn, a);
    }
    catch (xcept::Exception& e)
    {
        std::cout << "error during default memory pool creation: " << e.what() << std::endl;
        throw;
    }
}
