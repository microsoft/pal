/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implementation of the SCXLogHandle class.

    \date        07-06-07 14:27:00
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxlogitem.h>

namespace SCXCoreLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.
    
        Creates a default SCXLogItem.

    */
    SCXLogItem::SCXLogItem() :
        m_module(L""),
        m_severity(eNotSet),
        m_message(L""),
        m_location(L"", 0),
        m_threadId(0),
        m_timestamp(SCXCalendarTime::CurrentUTC())
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Constructor with parameters.
    
        \param[in]  module The string representation of the module the log
                           item belongs to.
        \param[in]  severity The severity of the new log item.
        \param[in]  message The actual log message.
        \param[in]  location Source code location.
        \param[in]  threadId - Thread that caused the log.
             
         Creates a new log item and sets the data members.

    */
    SCXLogItem::SCXLogItem(const std::wstring& module,
                           SCXLogSeverity severity,
                           const std::wstring& message,
                           const SCXCodeLocation& location,
                           SCXThreadId threadId) :
        m_module(module),
        m_severity(severity),
        m_message(message),
        m_location(location),
        m_threadId(threadId),
        m_timestamp(SCXCalendarTime::CurrentUTC())
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Copy constructor
    
        \param[in]  o SCXLogItem to copy.
    */
    SCXLogItem::SCXLogItem(const SCXLogItem& o) :
        m_module(o.m_module),
        m_severity(o.m_severity),
        m_message(o.m_message),
        m_location(o.m_location),
        m_threadId(o.m_threadId),
        m_timestamp(o.m_timestamp)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor.
    */
    SCXLogItem::~SCXLogItem()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Assignment operator
    
        \param[in]  o SCXLogItem to copy.
        \returns *this
    */
    SCXLogItem& SCXLogItem::operator=(const SCXLogItem& o)
    {
        m_module = o.m_module;
        m_severity = o.m_severity;
        m_message = o.m_message;
        m_location = o.m_location;
        m_threadId = o.m_threadId;
        m_timestamp = o.m_timestamp;
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Returns a string representation of the module this log item belongs to.
    
    */
    const std::wstring& SCXLogItem::GetModule() const
    {
        return m_module;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Returns the severity of this log item.
    
    */
    SCXLogSeverity SCXLogItem::GetSeverity() const
    {
        return m_severity;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Returns the log message as passed by the developer.
    
    */
    const std::wstring& SCXLogItem::GetMessage() const
    {
        return m_message;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Returns the source code location that generated this log item.
    
    */
    const SCXCodeLocation& SCXLogItem::GetLocation() const
    {
        return m_location;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Returns the thread id of the thread that generated this log item.
    
    */
    SCXThreadId SCXLogItem::GetThreadId() const
    {
        return m_threadId;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Returns the timestamp when the item was created.
    
    */
    const SCXCalendarTime& SCXLogItem::GetTimestamp() const
    {
        return m_timestamp;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Dump object as string (for logging).
    
        \retval   String representation of object.
        
    */
    const std::wstring SCXLogItem::DumpString() const
    {
        return SCXDumpStringBuilder("SCXLogItem")
            .Text("module", m_module)
            .Instance("timestamp", m_timestamp)
            .Scalar("severity", m_severity)
            .Text("message", m_message);
    }
        
}
