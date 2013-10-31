/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implementation for a file scxlog backend.

    \date        2008-07-23 15:15:04

*/
/*----------------------------------------------------------------------------*/

#include "scxlogfilebackend.h"
#include <scxcorelib/scxlogitem.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxproductdependencies.h>
#include <iomanip> // for setw

#include <stdexcept>
#include <stdlib.h>
#include <locale.h>
#if defined(SCX_UNIX)
#include <scxcorelib/scxuser.h>
#include <wctype.h>
#endif

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.
    */
    SCXLogFileBackend::SCXLogFileBackend() :
        SCXLogBackend(),
        m_FilePath(),
        m_LogFileRunningNumber(1),
        m_procStartTimestamp(SCXCalendarTime::CurrentUTC()),
        m_LogAllCharacters(false)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Constructor with filepath.
        \param[in] filePath Path to log file.
    */
    SCXLogFileBackend::SCXLogFileBackend(const SCXFilePath& filePath) :
        SCXLogBackend(),
        m_FilePath(filePath),
        m_FileStream(0),
        m_LogFileRunningNumber(1),
        m_procStartTimestamp(SCXCalendarTime::CurrentUTC()),
        m_LogAllCharacters(false)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Virtual destructor.
    */
    SCXLogFileBackend::~SCXLogFileBackend()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Add name of current user to file path.
    */
    void SCXLogFileBackend::AddUserNameToFilePath()
    {
#if defined(SCX_UNIX)
        SCXUser user;
        if (!user.IsRoot())
        {
            m_FilePath.AppendDirectory(user.GetName());
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        An SCXLogItem is submitted for output to this specific backend.
        When this method is called from LogThisItem, we are in the scope of a
        thread lock so there should be no need for one here.

        \param[in] item Log item to be submitted for output.
    */
    void SCXLogFileBackend::DoLogItem(const SCXLogItem& item)
    {
        if (m_FileStream == 0 || ! m_FileStream->is_open())
        {
            try {
                m_FileStream = SCXFile::OpenWFstream(m_FilePath, std::ios::out|std::ios::app);

                // Write a log file header
                SCXProductDependencies::WriteLogFileHeader( m_FileStream, m_LogFileRunningNumber, m_procStartTimestamp );
            }
            catch (const SCXFilePathNotFoundException&)
            {
                // We get this if we don't have permissions to create or write to this file.
                // There's not much we can do about this.
                return;
            }
            catch (const SCXUnauthorizedFileSystemAccessException&)
            {
                // We get this if we don't have permissions to create or write to this file.
                // There's not much we can do about this.
                return;
            }
        }

        std::wstring msg = Format(item);
        SCXProductDependencies::WrtieItemToLog( m_FileStream, item, msg );
    }

    /*----------------------------------------------------------------------------*/
    /**
       Handle log rotations that have occurred
     */
    void SCXLogFileBackend::HandleLogRotate()
    {
        m_LogFileRunningNumber++;
        m_FileStream->close();
        m_FileStream = 0;
        SCXLogItem item(L"scx.core.providers", eInfo, L"Log rotation complete", 
                        SCXSRCLOCATION, SCXThread::GetCurrentThreadID());
        DoLogItem(item);
    }

    /*----------------------------------------------------------------------------*/
    /**
        The backend can be configured using key - value pairs.

        \param[in] key Name of property to set.
        \param[in] value Value of property to set.
    */
    void SCXLogFileBackend::SetProperty(const std::wstring& key, const std::wstring& value)
    {
        if (L"PATH" == key)
        {
            m_FilePath.Set(value);
            //AddUserNameToFilePath();
        }

        if (L"LOGALLCHARACTERS" == key)
        {
            m_LogAllCharacters = true;
            //AddUserNameToFilePath();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        This implementation is initialized once the file path is not empty.

        \returns true if m_FilePath is not empty
    */
    bool SCXLogFileBackend::IsInitialized() const
    {
        return m_FilePath.Get().length() != 0;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the path to the log file.

        \returns Current path to log file.
    */
    const SCXFilePath& SCXLogFileBackend::GetFilePath() const
    {
        return m_FilePath;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Log format method.

        \param[in]  item An SCXLogItem to format.
        \returns    A formatted message:
                    "<time> <SEVERITY> [<module>:<linenumber>:<processid>:<threadid>] <message>"
    */
    const std::wstring SCXLogFileBackend::Format(const SCXLogItem& item) const
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

        // JWF -- Try and convert the string.  If this call fails, then we cannot print the string
        const std::wstring in_message(item.GetMessage());
        std::wstring message;

        bool messageHadUnprintable = false;

        ss << L" [" << item.GetModule() << L":" << item.GetLocation().WhichLine() << L":" << SCXProcess::GetCurrentProcessID() << L":" << item.GetThreadId() << L"] ";

        for (size_t i = 0; i < in_message.size(); i++)
        {
            wchar_t currentChar = in_message[i];

            if (!m_LogAllCharacters)
            {
                if (currentChar < 32 || currentChar > 126)
                {
                    ss << "[0x" << std::hex << std::setfill(L'0') << std::setw(3) << (unsigned short)currentChar << "]";
                    messageHadUnprintable = true;
                }
                else
                {
                    ss << currentChar;
                }
            }
            else
            {
                if (currentChar > 255)
                {
                    ss << "[0x" << std::hex << std::setfill(L'0') << std::setw(3) << (unsigned short)currentChar << "]";
                    messageHadUnprintable = true;
                }
                else
                {
                    ss << currentChar;
                }
            }
        }

        if (messageHadUnprintable)
        {
            ss << L" (* Message contained unprintable (?) characters *)";
        }

        return ss.str();
    }
} /* namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
