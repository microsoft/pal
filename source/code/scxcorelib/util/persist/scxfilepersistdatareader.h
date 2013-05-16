/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       File based implementation of the SCXPersistDataReader interface
    
    \date        2008-08-21 14:45:17

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXFILEPERSISTDATAREADER_H
#define SCXFILEPERSISTDATAREADER_H

#include <scxcorelib/scxpersistence.h>
#include <scxcorelib/scxfilepath.h>
#include <list>

namespace SCXCoreLib
{

    /*----------------------------------------------------------------------------*/
    /**
        File based implementation of the SCXPersistDataReader interface.
    */
    class SCXFilePersistDataReader : public SCXPersistDataReader
    {
    public:
        SCXFilePersistDataReader(const SCXFilePath& path);
        virtual ~SCXFilePersistDataReader();
        virtual unsigned int GetVersion();
        virtual bool ConsumeStartGroup(const std::wstring& name, bool dothrow = false);
        virtual bool ConsumeEndGroup(bool dothrow = false);
        virtual bool ConsumeValue(const std::wstring& name, std::wstring& value, bool dothrow = false);
        virtual std::wstring ConsumeValue(const std::wstring& name);
    private:
        wchar_t ConsumeEncodedChar();
        void Consume(const std::wstring& data);
        void ConsumeString(const std::wstring& data);
        std::wstring ConsumeString();
        wchar_t GetUTF8CharSkipLeadingWS();
        wchar_t GetUTF8Char();

        SCXHandle<std::fstream> m_Stream; //!< Stream for reading from file.
        std::list<std::wstring> m_StartedGroups; //!< Current open groups.
        unsigned int m_Version; //!< Version of persisted data as read from file.
        std::locale m_Locale; //!< Current locale (WI9509)
    };
}

#endif /* SCXFILEPERSISTDATAREADER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
