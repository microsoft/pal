/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the TestLogBackend class

    \date        2008-07-18 11:44:22

*/
/*----------------------------------------------------------------------------*/

#ifndef TESTLOGBACKEND_H
#define TESTLOGBACKEND_H

#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxlogitem.h>
#include "scxcorelib/util/log/scxlogbackend.h"

/**
    This class is a utility backend for testing purposes.
    Register it with the mediator and inspect what is being logged.
 */
class TestLogBackend : public SCXCoreLib::SCXLogBackend
{
public:
    TestLogBackend() :
        SCXCoreLib::SCXLogBackend(SCXCoreLib::ThreadLockHandleGet())
    {}

    TestLogBackend(const SCXCoreLib::SCXThreadLockHandle& lock) :
        SCXCoreLib::SCXLogBackend(lock)
    {}

    const SCXCoreLib::SCXLogItem& GetLastLogItem() const
    {
        return m_LastLogItem;
    }

    void SetProperty(const std::wstring& /*key*/, const std::wstring& /*value*/)
    {
    }

    bool IsInitialized() const
    {
        return true;
    }

protected:
    void DoLogItem(const SCXCoreLib::SCXLogItem& item)
    {
        m_LastLogItem = item;
    }

private:
    SCXCoreLib::SCXLogItem m_LastLogItem;
};

#endif
