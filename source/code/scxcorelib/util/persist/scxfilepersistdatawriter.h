/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       File based implementation of the SCXPersistDataWriter interface
    
    \date        2008-08-21 14:50:08

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXFILEPERSISTDATAWRITER_H
#define SCXFILEPERSISTDATAWRITER_H

#include <scxcorelib/scxpersistence.h>
#include <scxcorelib/scxfilepath.h>
#include <list>

namespace SCXCoreLib
{

    /*----------------------------------------------------------------------------*/
    /**
        File based implementation of the SCXPersistDataWriter interface.
    */
    class SCXFilePersistDataWriter : public SCXPersistDataWriter
    {
    public:
        SCXFilePersistDataWriter(const SCXFilePath& path, unsigned int version);
        virtual ~SCXFilePersistDataWriter();
        virtual void WriteStartGroup(const std::wstring& name);
        virtual void WriteEndGroup();
        virtual void WriteValue(const std::wstring& name, const std::wstring& value);
        virtual void DoneWriting();
    private:
        SCXHandle<std::fstream> m_Stream; //!< Stream for writing to file.
        std::list<std::wstring> m_StartedGroups; //!< Current open groups.
        std::wstring m_Indentation; //!< Contains spaces for indentation.
        std::wstring EncodeString(const std::wstring& in) const;
    };
}

#endif /* SCXFILEPERSISTDATAWRITER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
