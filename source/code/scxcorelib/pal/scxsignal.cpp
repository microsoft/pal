/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        scxsignal.cpp

    \brief       Real-time signal support for SCX.
    
    \date        05-02-2012 17:20:00
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxsignal.h>

#include <errno.h>

using namespace SCXCoreLib;

namespace SCXCoreLib
{
/*----------------------------------------------------------------------------*/
/**
    Constructor

    \param       sentinel       Sentinel to verify that we actually sent the signal
    \param       sig            [Optional] Signal number to use (default: SIGRTMIN)
*/
    SCXSignal::SCXSignal(u_short sentinel, int sig /* = SIGRTMIN */)
        : m_sigNumber(sig),
          m_magic(sentinel)
    {
    }

/*----------------------------------------------------------------------------*/
/**
    Destructor
*/
    SCXSignal::~SCXSignal()
    {
        // Unblock any signals that we might have blocked

        sigset_t unblockedSignals;
        sigemptyset( &unblockedSignals );
        sigaddset( &unblockedSignals, m_sigNumber );
        (void) sigprocmask(SIG_UNBLOCK, &unblockedSignals, NULL);
    }

/*----------------------------------------------------------------------------*/
/**
    Set thread to acceot signals and process them.  At least one thread
    (or the main thread) must allow signals to be accepted.

    You must pass a generic handler function.  This static function should
    simply call the (object)->Dispatcher method passing appropriate parameters.
    This is necessary to capture the object data for this class.

    \param      hndlrFunction   Static function to catch signal

    \throws      SCXInternalErrorException if called multiple times
*/
    void SCXSignal::AcceptSignals(hndlrFunction h)
    {
        // Unblock our signal in case it's already blocked

        sigset_t unblockedSignals;
        sigemptyset( &unblockedSignals );
        sigaddset( &unblockedSignals, m_sigNumber );
        int isError = sigprocmask(SIG_UNBLOCK, &unblockedSignals, NULL);

        if (isError)
        {
            throw SCXCoreLib::SCXErrnoException(L"sigprocmask", errno, SCXSRCLOCATION);
        }

        // Set up the signal action structure

        struct sigaction sa;
        memset( &sa, 0, sizeof(sa) );
        sa.sa_sigaction = *h;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset( &sa.sa_mask );

        // Process the signals we're supposed to accept
        if (sigaction(m_sigNumber, &sa, NULL) == -1)
        {
            throw SCXCoreLib::SCXErrnoException(L"sigaction", errno, SCXSRCLOCATION);
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Set thread to block signals and not process them.  Call this method to
    insure that only a specific set of threads can handle incoming signals.

    \throws      SCXInternalErrorException if called multiple times
*/
    void SCXSignal::BlockSignals()
    {
        sigset_t blockedSignals;
        sigemptyset( &blockedSignals );
        sigaddset( &blockedSignals, m_sigNumber );
        int isError = sigprocmask(SIG_BLOCK, &blockedSignals, NULL);

        if (isError)
        {
            throw SCXCoreLib::SCXErrnoException(L"sigprocmask", errno, SCXSRCLOCATION);
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Assign a handler for a signal.

    The SCXSignal class generally uses SIGRTMIN as a signal number (can be overridden
    via constructor, and then uses signal data (payload) to determine the type of signal
    to handle.  This allows a very large number of signals to be handled (currently 65535
    different signal types).

    There is an SCXSignal::Dispatcher method that will capture the signal from the O/S
    and then dispatch, based in the payload, to a method defined here.

    \param       payload        Handler number to assign a handler for
    \param       hndlrFunction  Pointer to function for handling this payload number

    \throws      SCXInvalidArgumentException if called multiple times with same payload
*/

    void SCXSignal::AssignHandler(u_short payload, void (*hndlrInstance)(siginfo_t *si))
    {
        // Exists?
        if (m_hndlrFunctions.count(payload) > 0)
        {
            throw SCXCoreLib::SCXInvalidArgumentException(
                L"payload", L"Payload already defined with a signal handler", SCXSRCLOCATION);
        }

        m_hndlrFunctions[payload] = hndlrInstance;
    }

/*----------------------------------------------------------------------------*/
/**
    Set thread to block signals and not process them.  Call this method to
    insure that only a specific set of threads can handle incoming signals.

    \param       pid            PID to deliver the signal to
    \param       payload        Signal payload identifier to signal
*/
    void SCXSignal::SendSignal(pid_t pid, u_short payload)
    {
        union sigval sv;

        // Encode the user data properly:
        //   Upper 16 bits = Sentinel
        //   Lower 16 bits = Payload

        unsigned int userData = m_magic;
        userData <<= 16;
        userData += payload;

        sv.sival_int = static_cast<int> (userData);

        // Signal
        if (sigqueue(pid, m_sigNumber, sv) == -1)
        {
            throw SCXCoreLib::SCXErrnoException(L"sigqueue", errno, SCXSRCLOCATION);
        }
    }

/*----------------------------------------------------------------------------*/
/**
    Set thread to block signals and not process them.  Call this method to
    insure that only a specific set of threads can handle incoming signals.

    \param       payload        Signal payload identifier to signal
*/
    void SCXSignal::Dispatcher(int sig, siginfo_t *si, void *ucontext)
    {
        (void) ucontext;

        if (sig == m_sigNumber)
        {
            int udata = si->si_value.sival_int;

            // Upper 16 bits is sentinal, lower 16 bits is the payload
            u_short sentinel = static_cast<u_short> ((static_cast<unsigned int> (udata) & 0xFFFF0000) >> 16);
            u_short payload = static_cast<u_short> ((static_cast<unsigned int> (udata) & 0xFFFF));

            // If this isn't our signal, just ignore it
            if ( m_magic != sentinel )
                return;

            // If we have this payload number in our map, then dispatch
            if (m_hndlrFunctions.count(payload) > 0)
            {
                (*m_hndlrFunctions[payload]) (si);
            }
        }
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
