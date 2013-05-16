/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Implementation of the SCXLogHandleFactory class.

    \date        07-06-08 11:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxlogpolicy.h>
#include "scxlogmediatorsimple.h"
#include "scxlogfileconfigurator.h"
#include <signal.h>
#include <errno.h>

namespace SCXCoreLib
{
#if !defined(DISABLE_WIN_UNSUPPORTED)  
    /*----------------------------------------------------------------------------*/
    /**
       Signal that a handler is registered for to react to occurred log rotates
       Currently we use SIGCONT because SIGHUP, SIGUSR1, SIGUSR2 causes the
       application to terminate, causing data loss since the data collection
       threads terminate. SIGUSR1 and SIGHUP terminates the application upon the
       first signal and SIGUSR2 terminates the application upon the second signal.
       When trying the same usage in a standalone program everything works fine,
       that is, the handler is called and the program doesn't terminate.
     */
    const int SCXLogHandleFactory::LOGROTATE_REACTION_SIGNAL = SIGCONT;
#endif //!defined(DISABLE_WIN_UNSUPPORTED)  
    /*----------------------------------------------------------------------------*/
    /**
        Creates and returns a new SCXLogHandle

        \param[in]  module A string representation of the module that will be
                           associated with the new log handle.
        \retval     An SCXLogHandle instance.
    */
    SCXLogHandle SCXLogHandleFactory::GetLogHandle(const std::wstring& module)
    {
        return SCXLogHandle(module, Instance().m_LogMediator, Instance().m_LogConfigurator);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieves a handle to the current log configurator.

        \retval     The log configurator used.
    */
    const SCXHandle<const SCXLogConfiguratorIf> SCXLogHandleFactory::GetLogConfigurator()
    {
        return Instance().m_LogConfigurator;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.

        Initializes the mediator to null.

    */
    SCXLogHandleFactory::SCXLogHandleFactory() :
        m_LogMediator(0),
        m_LogConfigurator(0)
    {
        SCXHandle<SCXLogMediator> m( new SCXLogMediatorSimple() );
        m_LogMediator = m;
        m_LogConfigurator =
            new SCXLogFileConfigurator(m, CustomLogPolicyFactory()->GetConfigFileName());
#if !defined(DISABLE_WIN_UNSUPPORTED)  
        InstallLogRotateSupport();
#endif //!defined(DISABLE_WIN_UNSUPPORTED)  
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor.

    */
    SCXLogHandleFactory::~SCXLogHandleFactory()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).

        \returns     String representation of object.

    */
    const std::wstring SCXLogHandleFactory::DumpString() const
    {
        return L"SCXLogHandleFactory";
    }

#if !defined(DISABLE_WIN_UNSUPPORTED)  
    /*----------------------------------------------------------------------------*/
    /**
       Pointer to the handler that was first in chain
       when we installed our log rotate handler.
       Should be called after we have done our work
     */
    SCXLogRotateHandlerPtr SCXLogHandleFactory::s_nextSignalHandler;


    /*----------------------------------------------------------------------------*/
    /**
       Make the application support log rotate by
       installing necessary signal handler.
     */
    void SCXLogHandleFactory::InstallLogRotateSupport() {
        struct sigaction action;
        sigemptyset(&action.sa_mask);
        action.sa_sigaction = 0;
        action.sa_handler = reinterpret_cast<SCXLogRotateHandlerPtr> (& HandleLogRotate);
        action.sa_flags = 0;
        struct sigaction priorAction;
        if (sigaction(LOGROTATE_REACTION_SIGNAL, &action, &priorAction) < 0) {
            SCXErrnoException(L"sigaction", errno, SCXSRCLOCATION);
        }
        s_nextSignalHandler = priorAction.sa_handler;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Handle log rotations that have occurred
       \param[in] sig  The received  signal
       \note This is a Posix signal handler
     */
    void SCXLogHandleFactory::HandleLogRotate(int sig) {
        SCXLogMediator &mediator = static_cast<SCXLogMediator &>(*Instance().m_LogMediator);
        mediator.HandleLogRotate();
        if (s_nextSignalHandler != 0) {
            sig = sig;
            s_nextSignalHandler(sig);
        }
    }
#endif //!defined(DISABLE_WIN_UNSUPPORTED)  

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
