/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        scxsignal.h

    \brief       Real time signal support for SCX.
    
    \date        05-02-2012 17:20:00
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXSIGNAL_H
#define SCXSIGNAL_H

#include <scxcorelib/scxcmn.h>

#include <map>
#include <signal.h>

namespace SCXCoreLib
{
    extern "C"
    {
        typedef void (*hndlrFunction)(int sig, siginfo_t *si, void *ucontext);
    }

    class SCXSignal
    {
    public:
        SCXSignal(u_short sentinel, int sig=SIGRTMIN);
        virtual ~SCXSignal();

        void AcceptSignals(hndlrFunction h);
        void BlockSignals();

        void AssignHandler(u_short payload, void (*hndlrInstance)(siginfo_t *si));
        void SendSignal(pid_t pid, u_short payload);

        void Dispatcher(int sig, siginfo_t *si, void *ucontext);

    protected:
        // Following entries are protected for unit test purposes only

        int m_sigNumber;                //!< Signal number to use (normally SIGRTMIN)
        u_short m_magic;                //!< Sentinel: Make sure we sent signal
        std::map<u_short, void (*)(siginfo_t *)> m_hndlrFunctions; //!< Registered handlers for various signals

    private:
        SCXSignal();                    //!< Intentially not implemented
    };
}

#endif /* SCXSIGNAL_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
