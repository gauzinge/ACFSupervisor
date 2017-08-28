#ifndef _DTC_StateMachine_h__
#define _DTC_StateMachine_h__

#include "xdaq/Application.h"
#include "xdaq/NamespaceURI.h"

#include "toolbox/fsm/FiniteStateMachine.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/task/Action.h"

#include "xoap/MessageReference.h"
#include "xoap/MessageFactory.h"
#include "xoap/Method.h"

#include "xdata/String.h"

#include "xdaq2rc/RcmsStateNotifier.h"

#include <string>

namespace Ph2TkDAQ {

    using State = char;

    class DTCStateMachine : public toolbox::lang::Class
    {
      public:
        DTCStateMachine (xdaq::Application* pApp);
        virtual ~DTCStateMachine();

      protected:
        toolbox::fsm::FiniteStateMachine fFSM;

      private:

    };
}

#endif
