/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Contains the definition of the Singleton Template.

    \date        2007-06-19 08:50:28

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXSINGLETONDEFS_H
#define SCXSINGLETONDEFS_H

#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxhandle.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Provides singleton functionality (definitions).

        \date        <2007-06-19 08:50:28>

        This class exists for definition purposes, please see scxsingleton.h
    */

    /*----------------------------------------------------------------------------*/
    /**
       Returns a reference to the singleton instance.

       If the instance is not yet created, it first creates it.

    */
    template<class T> T& SCXSingleton<T>::Instance()
    {
        if (0 == s_lockHandle.GetData())
        {
            throw SCXInternalErrorException(L"Tried to get a singleton instance before static initialization was completed.", SCXSRCLOCATION);
        }

        SCXThreadLock lock(*s_lockHandle, true);

        if (0 == s_instance.GetData())
        {
            s_instance = new T;
        }

        return *s_instance;
    }

    template<class T> SCXSingleton<T>::SCXSingleton(int recursiveLock /* = 1 */)
    {
        if (recursiveLock == 0)
        {
            s_lockHandle = new SCXThreadLockHandle(ThreadLockHandleGet(recursiveLock));
        }
    }
}

#endif /* SCXSINGLETONDEF_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
