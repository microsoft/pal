/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Implementation of the SCXFilePersistDataWriter class.
    
    \date        2008-08-21 15:06:10

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include "scxfilepersistdatawriter.h"
#include <string>
#include <sstream>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Constructor

        \param[in]  path        Path where persisted data should be written.
        \param[in]  version     Version of data stored.
    */
    SCXFilePersistDataWriter::SCXFilePersistDataWriter(const SCXFilePath& path, unsigned int version) :
        SCXPersistDataWriter(version),
        m_StartedGroups(),
        m_Indentation(L"  ")
    {
        m_Stream = SCXFile::OpenFstream(path, std::ios_base::out);
        std::wostringstream os;
        os << L"<?xml version=\"1.0\" encoding='UTF-8' standalone='yes' ?>" << std::endl;
        os << L"<SCXPersistedData Version=\"" << version << L"\">" << std::endl;
        SCXStream::WriteAsUTF8(*m_Stream, os.str());
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor
    */
    SCXFilePersistDataWriter::~SCXFilePersistDataWriter()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Mark the start of a new group.

        \param[in]  name     Name of group.
    */
    void SCXFilePersistDataWriter::WriteStartGroup(const std::wstring& name)
    {
        std::wostringstream os;
        os << m_Indentation << L"<Group Name=\"" << EncodeString(name) << L"\">" << std::endl;
        SCXStream::WriteAsUTF8(*m_Stream, os.str());    
        m_StartedGroups.push_front(name);
        m_Indentation.append(L"  ");
    }

    /*----------------------------------------------------------------------------*/
    /**
        Mark the end of the last started group.
        \throws SCXInvalidStateException if no group is currently started.
    */
    void SCXFilePersistDataWriter::WriteEndGroup()
    {
        if (m_StartedGroups.empty())
        {
            throw SCXInvalidStateException(L"No open group when calling WriteEndGroup.", SCXSRCLOCATION);
        }
        m_Indentation.erase(0, 2);
        std::wostringstream os;
        os << m_Indentation << L"</Group>" << std::endl;
        SCXStream::WriteAsUTF8(*m_Stream, os.str());
        m_StartedGroups.pop_front();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Write a new name/value pair.

        \param[in]  name    Name of value.
        \param[in] value    Value.
    */
    void SCXFilePersistDataWriter::WriteValue(const std::wstring& name, const std::wstring& value)
    {
        std::wostringstream os;
        os << m_Indentation  << L"<Value Name=\""   << EncodeString(name)
           << L"\" Value=\"" << EncodeString(value) << L"\"/>" << std::endl;
        SCXStream::WriteAsUTF8(*m_Stream, os.str());
    }

    /*----------------------------------------------------------------------------*/
    /**
        Mark the end of writing data.
        Will also be called from destructor if not called explicitly
        \throws SCXInvalidStateException if all groups have not been ended.
    */
    void SCXFilePersistDataWriter::DoneWriting()
    {
        if ( ! m_StartedGroups.empty())
        {
            throw SCXInvalidStateException(L"Can not call DoneWriting when open groups still exist.", SCXSRCLOCATION);
        }
        if (m_Stream->is_open())
        {
            SCXStream::WriteAsUTF8(*m_Stream, L"</SCXPersistedData>\n");
            m_Stream->close();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Encode a string replacing special characters with their xml counterparts.
        \param[in] in String to encode.
        \returns Encoded string.
    */
    std::wstring SCXFilePersistDataWriter::EncodeString(const std::wstring& in) const
    {
        std::wstring out;
        for (std::wstring::const_iterator i = in.begin(); i != in.end(); ++i)
        {
            switch (*i)
            {
            case L'<':
                out.append(L"&lt;");
                break;
            case L'&':
                out.append(L"&amp;");
                break;
            case L'\'':
                out.append(L"&apos;");
                break;
            case L'\"':
                out.append(L"&quot;");
                break;
            default:
                out.push_back(*i);
                break;
            }
        }
        return out;
    }
}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
