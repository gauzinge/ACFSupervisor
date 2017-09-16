#ifndef __I2OMSG_H
#define __I2OMSG_H

#include "i2o/shared/i20msg.h"
#include "i2o/Method.h"
#include "toolbox/mem/Reference.h"
#include "interface/evb/i2oEVBMsgs.h"

#include <iostream>


class I2OMsg
{
  public:
    I2OMsg();
    ~I2OMsg();

    void* getBuffer();
    void* getTrailingBuffer();

    //implemented from pixel::I2OStandardMessage
    void initialiseDefaultValues();
    int getStandardHeaderSize();
    int getPrivateHeaderSize();
    int getEventDataBlockHeaderSize();

    PI2O_MESSAGE_FRAME getStandardMessageFrame();
    PI2O_PRIVATE_MESSAGE_FRAME getPrivateMessageFrame();
    PI2O_EVENT_DATA_BLOCK_MESSAGE_FRAME getEventDataBlockMessageFrame();

    uint8_t getVersionOffset();
    void setVersionOffset (uint8_t pVersionOffset);

    uint8_t getMsgFlags();
    void setMsgFlags (uint8_t pMsgFlags);

    uint16_t getMsgSize();
    void setMsgSize (uint16_t pMsgSize);

    BF getTargetAddress();
    void setTargetAddress (BF pTargetAddress);

    BF getInitiatorAddress();
    void setInitiatorAddress (BF pInitiatorAddress);

    BF getFunction();
    void setFunction (BF pFunction);

    I2O_INITIATOR_CONTEXT getInitiatorContext();
    void setInitiatorContext (I2O_INITIATOR_CONTEXT pContext);

    I2O_TRANSACTION_CONTEXT getTransactionContext();
    void setTransactionContext (I2O_TRANSACTION_CONTEXT pContext);

    uint16_t getXFunction();
    void setXFunction (uint16_t pFunctionCode);

    uint16_t getOrganizationID();
    void setOrganizationID (uint16_t pOrganizationID);

    //from EventData Block
    uint32_t getEventNumber();
    void setEventNumber (uint32_t pEventNumber);

    uint32_t getNbBlocksInSuperFragment();
    void setNbBlocksInSuperFragment (uint32_t pNbBlocks);

    uint32_t getBlockNb();
    void setBlockNb (uint32_t pBlockNb);

    uint32_t getEventId();
    void setEventId (uint32_t pEventId);

    uint32_t getBuResourceId();
    void setBuResourceId (uint32_t pBuResorceId);

    uint32_t getFuTransactionId();
    void setFuTransactionId (uint32_t pTransactionId);

    uint32_t getNbSuperFragmentsInEvent();
    void setNbSuperFragmentsInEvent (uint32_t pNbSuperFragments);

    uint32_t getSuperFragmentNb();
    void setSuperFragmentNb (uint32_t pSuperFragmentNb);

    uint32_t getPadding();
    void setPadding (uint32_t pPadding);


  protected:
    void clear();
    void setBuffer (void* pBuffer);
    void setReference (toolbox::mem::Reference* pRef, int pAutoRelease = 1);
    toolbox::mem::Reference* getReference();

  private:
    toolbox::mem::Reference* fRef;
    void* fBuffer;
    int fAutoRelease;
};

std::ostream& operator << (std::ostream& out, I2OMsg& pMsg);


#endif
