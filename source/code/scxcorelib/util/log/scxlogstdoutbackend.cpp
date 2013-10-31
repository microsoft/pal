/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implementation for an stdout scxlog backend.

    \date        2008-08-05 13:16:34

*/
/*----------------------------------------------------------------------------*/

#include "scxlogstdoutbackend.h"
#include <scxcorelib/scxlogitem.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.
    */
    SCXLogStdoutBackend::SCXLogStdoutBackend() :
        SCXLogBackend()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor.
    */
    SCXLogStdoutBackend::~SCXLogStdoutBackend()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        An SCXLogItem is submitted for output to this specific backend.
        When this method is called from LogThisItem, we are in the scope of a
        thread lock so there should be no need for one here.

        \param[in] item Log item to be submitted for output.
    */
    void SCXLogStdoutBackend::DoLogItem(const SCXLogItem& item)
    {
        std::wstring msg = Format(item);
        std::wcout << msg << std::endl;
    }

    /**
        The backend can be configured using key - value pairs.
        This implementation does not care about any properties.
            
        \param[in] key Name of property to set.
        \param[in] value Value of property to set.
    */
    void SCXLogStdoutBackend::SetProperty(const std::wstring& /*key*/, const std::wstring& /*value*/)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        This implementation is always initialized.

        \returns true
    */
    bool SCXLogStdoutBackend::IsInitialized() const
    {
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Log format method.

        \param[in]  item An SCXLogItem to format.
        \returns    A formatted message:
                    "<time> <SEVERITY> [<module>:<linenumber>:<processid>:<threadid>] <message>"
    */
    const std::wstring SCXLogStdoutBackend::Format(const SCXLogItem& item) const
    {
        static const wchar_t* severityStrings[] = {
            L"NotSet    ",
            L"Hysterical",
            L"Trace     ",
            L"Info      ",
            L"Warning   ",
            L"Error     "
        };

        std::wstringstream ss;

        ss << item.GetTimestamp().ToExtendedISO8601() << L" ";

        if (item.GetSeverity() > eError)
        {
            ss << L"Unknown " << item.GetSeverity();
        }
        else
        {
            ss << severityStrings[item.GetSeverity()];
        }

        ss << L" [" << item.GetModule() << L":" << item.GetLocation().WhichLine() << L":" << SCXProcess::GetCurrentProcessID() << L":" << item.GetThreadId() << L"] " << item.GetMessage();

        return ss.str();
    }
} /* namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
