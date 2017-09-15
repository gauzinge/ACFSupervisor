#include "DTCSupervisor/I2OMsg.h"

//public
I2OMsg::I2OMsg()
{
    fRef = nullptr;
    fBuffer = nullptr;
    pAutoRelease = 0;
}

I2OMsg::~I2OMsg()
{
    this->clear();
}
void* I2OMsg::getBuffer()
{
    return fBuffer;
}
void* I2OMsg::getTrailingBuffer()
{
    return (void*) ( (intptr_t) getBuffer() + getHeaderSize() );
}
void I2OMsg::initialiseDefaultValues()
{
    this->setVersionOffset (0);
    this->setMsgFlags (0);
    this->setTargetAddress (0);
    this->setInitiatorAddress (0);
    this->setMsgSize (this->getHeaderSize() >> 2);
    this->setFunction (I2O_PRIVATE_MESSAGE);
    I2O_TRANSACTION_CONTEXT cCt;
    memset (&cCt, 0, sizeof (I2O_TRANSACTION_CONTEXT) );
    this->setTransactionContext (cCt);
    this->setXFunction (0);
    this->setOrganizationID (XDAQ_ORGANIZATION_ID);
    this->setEventNumber (0);
    this->setNbBlocksInSuperFragment (1);
    this->setBlockNb (0);
    this->setBuResourceId (0);
    this->setFuTransactionId (0);
    this->setSuperFragmentNb (1);
    this->setPadding (0);
}
int I2OMsg::getStandardHeaderSize()
{
    return sizeof (PI2O_MESSAGE_FRAME);
}
int I2OMsg::getPrivateHeaderSize()
{
    return sizeof (PI2O_PRIVATE_MESSAGE_FRAME);
}
int I2OMsg::getEventDataBlockHeaderSize()
{
    return sizeof (PI2O_EVENT_DATA_BLOCK_MESSAGE_FRAME);
}
PI2O_MESSAGE_FRAME I2OMsg::getStandardMessageFrame()
{
    return (PI2O_MESSAGE_FRAME) this->getBuffer();
}
PI2O_PRIVATE_MESSAGE_FRAME I2OMsg::getPrivateMessageFrame()
{
    return (PI2O_PRIVATE_MESSAGE_FRAME) this->getBuffer();
}
PI2O_EVENT_DATA_BLOCK_MESSAGE_FRAME I2OMsg::getEventDataBlockMessageFrame()
{
    return (PI2O_EVENT_DATA_BLOCK_MESSAGE_FRAME) this->getBuffer();
}
uint8_t I2OMsg::getVersionOffset()
{
    return this->getStandardMessageFrame()->VersionOffset;
}
void I2OMsg::setVersionOffset (uint8_t pVersionOffset)
{
    this->getStandardMessageFrame()->VersionOffset = pVersionOffset;
}
uint8_t I2OMsg::getMsgFlags()
{
    return this->getStandardMessageFrame()->MsgFlags;
}
void I2OMsg::setMsgFlags (uint8_t pMsgFlags)
{
    this->getStandardMessageFrame()->MsgFlags = pMsgFlags;
}
uint16_t I2OMsg::getMsgSize()
{
    return this->getStandardMessageFrame()->MessageSize;
}
void I2OMsg::setMsgSize (uint16_t pMsgSize)
{
    this->getStandardMessageFrame()->MessageSize = pMsgSize;
}
BF I2OMsg::getTargetAddress()
{
    return this->getStandardMessageFrame()->TargetAddress;
}
void I2OMsg::setTargetAddress (BF pTargetAddress)
{
    this->getStandardMessageFrame()->TargetAddress = pTargetAddress;
}
BF I2OMsg::getInitiatorAddress()
{
    return this->getStandardMessageFrame()->InitiatorAddress;
}

void I2OMsg::setInitiatorAddress (BF pInitiatorAddress)
{
    this->getStandardMessageFrame()->InitiatorAddress = pInitiatorAddress;
}
BF I2OMsg::getFunction()
{
    return this->getStandardMessageFrame()->Function;
}
void I2OMsg::setFunction (BF pFunction)
{
    this->getStandardMessageFrame()->Function = pFunction;
}
I2O_TRANSACTION_CONTEXT I2OMsg::getTransactionContext()
{
    return this->getPrivateMessageFrame()->TransactionContext;
}
void I2OMsg::setTransactionContext (I2O_TRANSACTION_CONTEXT pContext)
{
    this->getPrivateMessageFrame()->TransactionContext = pContext;
}
uint16_t I2OMsg::getXFunction()
{
    return this->getPrivateMessageFrame()->XFunctionCode;
}
void I2OMsg::setXFunction (uint16_t pFunctionCode)
{
    this->getPrivateMessageFrame()->XFunctionCode = pFunctionCode;
}
uint16_t I2OMsg::getOrganizationID()
{
    return this->getPrivateMessageFrame()->OrganizationID;
}
void I2OMsg::setOrganizationID (uint16_t pOrganizationID)
{
    this->getPrivateMessageFrame()->OrganizationID = pOrganizationID;
}
//from EventDataBlock
uint32_t I2OMsg::getEventNumber()
{
    return this->getEventDataBlockMessageFrame()->eventNumber;
}
void I2OMsg::setEventNumber (uint32_t pEventNumber)
{
    this->getEventDataBlockMessageFrame()->eventNumber = pEventNumber;
}
uint32_t I2OMsg::getNbBlocksInSuperFragment()
{
    return this->getEventDataBlockMessageFrame()->nbBlocksInSuperFragment;
}
void I2OMsg::setNbBlocksInSuperFragment (uint32_t pNbBlocks)
{
    this->getEventDataBlockMessageFrame()->nbBlocksInSuperFragment = pNbBlocks;
}
uint32_t I2OMsg::getBlockNb()
{
    return this->getEventDataBlockMessageFrame()->blockNb;
}
void I2OMsg::setBlockNb (uint32_t pBlockNb)
{
    this->getEventDataBlockMessageFrame()->blockNb = pBlockNb;
}
uint32_t I2OMsg::getEventId()
{
    return this->getEventDataBlockMessageFrame()->eventId;
}

void I2OMsg::setEventId (uint32_t pEventId)
{
    getEventDataBlockMessageFrame()->eventId = pEventId;
}

uint32_t I2OMsg::getBuResourceId()
{
    return this->getEventDataBlockMessageFrame()->buResourceId;
}

void I2OMsg::setBuResourceId (uint32_t pBuResorceId)
{
    this->getEventDataBlockMessageFrame()->buResourceId = pBuResorceId;
}

uint32_t I2OMsg::getFuTransactionId()
{
    return this->getEventDataBlockMessageFrame()->fuTransactionId;
}

void I2OMsg::setFuTransactionId (uint32_t pTransactionId)
{
    this->getEventDataBlockMessageFrame()->fuTransactionId = pTransactionId;
}

uint32_t I2OMsg::getNbSuperFragmentsInEvent()
{
    return this->getEventDataBlockMessageFrame()->nbSuperFragmentsInEvent;
}

void I2OMsg::setNbSuperFragmentsInEvent (uint32_t pNbSuperFragments)
{
    this->getEventDataBlockMessageFrame()->nbSuperFragmentsInEvent = pNbSuperFragments;
}

uint32_t I2OMsg::getSuperFragmentNb()
{
    return this->getEventDataBlockMessageFrame()->superFragmentNb;
}

void I2OMsg::setSuperFragmentNb (uint32_t pSuperFragmentNb)
{
    this->getEventDataBlockMessageFrame()->superFragmentNb = pSuperFragmentNb;
}

uint32_t I2OMsg::getPadding()
{
    assert (0);
    //padding removed from struct... don't know how to fix! (ryd)
    //  return getEventDataBlockMessageFrame()->padding;
    return 0;
}

void I2OMsg::setPadding (uint32_t pPadding)
{
    assert (0);
    //padding removed from struct... don't know how to fix! (ryd)
    //   getEventDataBlockMessageFrame()->padding = padding

}

//protected
void I2OMsg::clear()
{
    if (fAutoRelease)
    {
        if (fRef)
            fRef->release();
        else
        {
            //see later
        }
    }

    fRef = 0;
}
void I2OMsg::setBuffer (void* pBuffer)
{
    fBuffer = pBuffer;
}
void I2OMsg::setReference (toolbox::mem::Reference* pRef, int pAutoRelease)
{
    if (fRef)
        this->clear();

    fRef = pRef;
    fAutoRelease = pAutoRelease;
    fBuffer = pRef->getDataLocation();
}
toolbox::mem::Reference* I2OMsg::getReference()
{
    return fRef;
}

std::ostream& operator<< (std::ostream& out, I2OMsg& pMsg)
{
    out << "I2OMsg" << std::endl;
    out << "Message size in 32 bit words: " << pMsg.getMsgSize() << std::endl;
    out << "Initiator Address: " << pMsg.getInitiatorAddress() << std::endl;
    out << "Target Address: " << pMsg.getTargetAddress() << std::endl;
    out << "XFunction Code: " << pMsg.getXFunction() << std::endl;
    out << "OrganizationID: " << pMsg.getOrganizationID() << std::endl;
    out << "EventNumber: " << pMsg.getEventNumber() << std::endl;
    return out;
}
