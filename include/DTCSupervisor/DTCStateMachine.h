/*
  This file is part of Fec Software project.

  Fec Software is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Fec Software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Fec Software; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright 2002 - 2003, P. Schieferdecker - CERN
  Changed by F. Drouhin to add more state, namespace is changed to avoid confusion
  with the other StateMachin, changed again by Jonathan Fulcher to use for Fed to DTCStateMachine
  changed again by G. Auzinger for application for the Ph2 Tracker DAQ

*/
#ifndef _DTC_StateMachine_h__
#define _DTC_StateMachine_h__

#include "xdaq/WebApplication.h"
#include "xdaq/Application.h"
#include "xdaq/NamespaceURI.h"
#include "toolbox/fsm/FailedEvent.h"

#include "toolbox/fsm/FiniteStateMachine.h"
#include "toolbox/task/WorkLoopFactory.h"
#include "toolbox/task/Action.h"
#include "toolbox/fsm/FailedEvent.h"

#include "xoap/MessageReference.h"
#include "xoap/MessageFactory.h"
#include "xoap/Method.h"

#include "xdata/String.h"
#include "xdata/Boolean.h"

#include "xdaq2rc/RcmsStateNotifier.h"

#include "xoap/SOAPEnvelope.h"
#include "xoap/SOAPBody.h"
#include "xoap/domutils.h"

#include "xcept/tools.h"

#include <typeinfo>
#include <string>
#include <sstream>

namespace Ph2TkDAQ {

    using State = char;

    class DTCStateMachine : public toolbox::lang::Class
    {
      public:
        DTCStateMachine (xdaq::Application* pApp);
        virtual ~DTCStateMachine();

        // finite state machine command callback
        xoap::MessageReference commandCallback (xoap::MessageReference msg)
        throw (xoap::exception::Exception);

        // finite state machine callback for entering new state
        void stateChanged (toolbox::fsm::FiniteStateMachine& fsm)
        throw (toolbox::fsm::exception::Exception);

        // finite state machine callback for transition into 'Failed' state
        void failed (toolbox::Event::Reference e)
        throw (toolbox::fsm::exception::Exception);

        // fire state transition event
        void fireEvent (const std::string& evtType, void* originator);

        // initiate transition to state 'Failed'
        void fireFailed (const std::string& errorMsg, void* originator);

        // report current state
        xdata::String* stateName()
        {
            return &stateName_;
        }

        // lookup the RCMS state listener
        void findRcmsStateListener()
        {
            rcmsStateNotifier_.findRcmsStateListener();
        }

        // report if RCMS StateListener was found
        xdata::Boolean* foundRcmsStateListener()
        {
            return rcmsStateNotifier_.getFoundRcmsStateListenerParameter();
        }

        // initialize state machine and bind callbacks to driving application
        template<class T> void initialize (T* app)
        {
            // action signatures
            asInitialising_   = toolbox::task::bind (app, &T::initialising,  "initialising");
            asConfiguring_    = toolbox::task::bind (app, &T::configuring,   "configuring");
            asEnabling_       = toolbox::task::bind (app, &T::enabling,      "enabling");
            asStopping_       = toolbox::task::bind (app, &T::stopping,      "stopping");
            asHalting_        = toolbox::task::bind (app, &T::halting,       "halting");
            asDestroying_     = toolbox::task::bind (app, &T::destroying,    "destroying");
            asPausing_        = toolbox::task::bind (app, &T::pausing,       "pausing");
            asResuming_       = toolbox::task::bind (app, &T::resuming,      "resuming");

            // work loops
            workLoopInitialising_ =
                toolbox::task::getWorkLoopFactory()->getWorkLoop (appNameAndInstance_ +
                        "_Initialising",
                        "waiting");
            workLoopConfiguring_ =
                toolbox::task::getWorkLoopFactory()->getWorkLoop (appNameAndInstance_ +
                        "_Configuring",
                        "waiting");
            workLoopEnabling_ =
                toolbox::task::getWorkLoopFactory()->getWorkLoop (appNameAndInstance_ +
                        "_Enabling",
                        "waiting");
            workLoopStopping_ =
                toolbox::task::getWorkLoopFactory()->getWorkLoop (appNameAndInstance_ +
                        "_Stopping",
                        "waiting");
            workLoopHalting_ =
                toolbox::task::getWorkLoopFactory()->getWorkLoop (appNameAndInstance_ +
                        "_Halting",
                        "waiting");
            workLoopDestroying_ =
                toolbox::task::getWorkLoopFactory()->getWorkLoop (appNameAndInstance_ +
                        "_Destroying",
                        "waiting");
            workLoopPausing_ =
                toolbox::task::getWorkLoopFactory()->getWorkLoop (appNameAndInstance_ +
                        "_Pausing",
                        "waiting");
            workLoopResuming_ =
                toolbox::task::getWorkLoopFactory()->getWorkLoop (appNameAndInstance_ +
                        "_Resuming",
                        "waiting");




            // bind SOAP callbacks
            //xoap::bind (app, &T::fsmCallback, "Initialise",   XDAQ_NS_URI);
            //xoap::bind (app, &T::fsmCallback, "Initialize",   XDAQ_NS_URI);
            //xoap::bind (app, &T::fsmCallback, "Configure",    XDAQ_NS_URI);
            //xoap::bind (app, &T::fsmCallback, "Enable",       XDAQ_NS_URI);
            //xoap::bind (app, &T::fsmCallback, "Halt",         XDAQ_NS_URI);
            //xoap::bind (app, &T::fsmCallback, "Stop",         XDAQ_NS_URI);
            //xoap::bind (app, &T::fsmCallback, "Destroy",      XDAQ_NS_URI);
            //xoap::bind (app, &T::fsmCallback, "Pause",        XDAQ_NS_URI);
            //xoap::bind (app, &T::fsmCallback, "Resume",       XDAQ_NS_URI);


            // define finite state machine
            fsm_.addState ('I', "Initial", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('i', "initialising", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('C', "Configured", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('c', "configuring", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('E', "Enabled", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('e', "enabling", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('s', "stopping", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('H', "Halted", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('h', "halting", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('x', "destroying", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('P', "Paused", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('p', "pausing", this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.addState ('r', "resuming", this, &DTCStateMachine::DTCStateMachine::stateChanged);






            // define the transition
            fsm_.addStateTransition ('I', 'i', "Initialise");
            fsm_.addStateTransition ('i', 'H', "InitialiseDone");
            //fsm_.addStateTransition ('i', 'H', "InitialiseDone", app, &T::Initialised);

            fsm_.addStateTransition ('H', 'c', "Configure");
            fsm_.addStateTransition ('c', 'C', "ConfigureDone");
            //fsm_.addStateTransition ('c', 'C', "ConfigureDone", app, &T::Configured);

            fsm_.addStateTransition ('C', 'e', "Enable");
            fsm_.addStateTransition ('e', 'E', "EnableDone");

            fsm_.addStateTransition ('C', 'h', "Halt");
            fsm_.addStateTransition ('E', 'h', "Halt");
            fsm_.addStateTransition ('P', 'h', "Halt");
            fsm_.addStateTransition ('h', 'H', "HaltDone");

            fsm_.addStateTransition ('E', 's', "Stop");
            fsm_.addStateTransition ('P', 's', "Stop");
            fsm_.addStateTransition ('s', 'C', "StopDone");

            fsm_.addStateTransition ('H', 'x', "Destroy");
            fsm_.addStateTransition ('x', 'I', "DestroyDone");

            fsm_.addStateTransition ('E', 'p', "Pause");
            fsm_.addStateTransition ('p', 'P', "PauseDone");

            fsm_.addStateTransition ('P', 'r', "Resume");
            fsm_.addStateTransition ('r', 'E', "ResumeDone");



            fsm_.addStateTransition ('i', 'F', "Fail", this, &DTCStateMachine::DTCStateMachine::failed);
            fsm_.addStateTransition ('c', 'F', "Fail", this, &DTCStateMachine::DTCStateMachine::failed);
            fsm_.addStateTransition ('e', 'F', "Fail", this, &DTCStateMachine::DTCStateMachine::failed);
            fsm_.addStateTransition ('s', 'F', "Fail", this, &DTCStateMachine::DTCStateMachine::failed);
            fsm_.addStateTransition ('h', 'F', "Fail", this, &DTCStateMachine::DTCStateMachine::failed);
            fsm_.addStateTransition ('F', 'F', "Fail", this, &DTCStateMachine::DTCStateMachine::failed);
            fsm_.addStateTransition ('p', 'F', "Fail", this, &DTCStateMachine::DTCStateMachine::failed);
            fsm_.addStateTransition ('r', 'F', "Fail", this, &DTCStateMachine::DTCStateMachine::failed);



            fsm_.setFailedStateTransitionAction (this, &DTCStateMachine::DTCStateMachine::failed);
            fsm_.setFailedStateTransitionChanged (this, &DTCStateMachine::DTCStateMachine::stateChanged);
            fsm_.setStateName ('F', "Failed");

            fsm_.setInitialState ('I');
            fsm_.reset();
            stateName_ = fsm_.getStateName (fsm_.getCurrentState() );

            if (!workLoopInitialising_->isActive() ) workLoopInitialising_->activate();

            if (!workLoopConfiguring_->isActive() ) workLoopConfiguring_->activate();

            if (!workLoopEnabling_->isActive() ) workLoopEnabling_->activate();

            if (!workLoopStopping_->isActive() ) workLoopStopping_->activate();

            if (!workLoopHalting_->isActive() ) workLoopHalting_->activate();

            if (!workLoopDestroying_->isActive() ) workLoopDestroying_->activate();

            if (!workLoopPausing_->isActive() ) workLoopPausing_->activate();

            if (!workLoopResuming_->isActive() ) workLoopResuming_->activate();


            findRcmsStateListener();

            //Export the stateName variable
            app->getApplicationInfoSpace()->fireItemAvailable ("stateName", &stateName_);
        }

        /** Get state name
         */
        std::string getStateName (State s) throw (toolbox::fsm::exception::Exception)
        {
            return fsm_.getStateName (s) ;
        }

        /** Set a state name
         */
        void setStateName (State s, const std::string& name) throw (toolbox::fsm::exception::Exception)
        {

            fsm_.setStateName (s, name) ;
        }

        /** Get all states
         */
        std::vector<State> getStates()
        {
            return fsm_.getStates() ;
        }

        /** Return input for a given state
         */
        std::set<std::string> getInputs (State s)
        {

            return fsm_.getInputs (s) ;
        }

        /** Retun all inputs
         */
        std::set<std::string> getInputs()
        {
            return fsm_.getInputs() ;
        }

        /** return the possible transitions for all inputs from a given state
         */
        std::map<std::string, State, std::less<std::string> > getTransitions (State s) throw (toolbox::fsm::exception::Exception)
        {

            return fsm_.getTransitions (s) ;
        }

        /** Get the current state
         */
        State getCurrentState()
        {

            return fsm_.getCurrentState() ;
        }

        /** Get the finite state machine
         */
        toolbox::fsm::FiniteStateMachine getXDAQFSM ( )
        {
            return fsm_ ;
        }


      protected:
        toolbox::fsm::FiniteStateMachine fsm_;

      private:
        log4cplus::Logger                logger_;
        std::string                      appNameAndInstance_;
        xdata::String                    stateName_;

        // work loops for transitional states
        toolbox::task::WorkLoop*         workLoopInitialising_;
        toolbox::task::WorkLoop*         workLoopConfiguring_;
        toolbox::task::WorkLoop*         workLoopEnabling_;
        toolbox::task::WorkLoop*         workLoopStopping_;
        toolbox::task::WorkLoop*         workLoopHalting_;
        toolbox::task::WorkLoop*         workLoopDestroying_;
        toolbox::task::WorkLoop*         workLoopDcuConfiguring_;
        toolbox::task::WorkLoop*         workLoopPausing_;
        toolbox::task::WorkLoop*         workLoopResuming_;

        // action signatures for transitional states
        toolbox::task::ActionSignature*  asInitialising_ ;
        toolbox::task::ActionSignature*  asConfiguring_;
        toolbox::task::ActionSignature*  asEnabling_;
        toolbox::task::ActionSignature*  asStopping_;
        toolbox::task::ActionSignature*  asHalting_;
        toolbox::task::ActionSignature*  asDestroying_;
        toolbox::task::ActionSignature*  asDcuConfiguring_;
        toolbox::task::ActionSignature*  asPausing_;
        toolbox::task::ActionSignature*  asResuming_;

        // rcms state notifier
        xdaq2rc::RcmsStateNotifier       rcmsStateNotifier_;
    };
}

#endif
