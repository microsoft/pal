/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       File based implementation of the SCXPersistMedia interface
    
    \date        2008-08-21 16:23:44

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXFILEPERSISTMEDIA_H
#define SCXFILEPERSISTMEDIA_H


#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxpersistence.h>
#include <scxcorelib/scxfilepath.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Simple file based implementation of SCXPersistMedia interface.
    */
    class SCXFilePersistMedia : public SCXPersistMedia
    {
    public:
        SCXFilePersistMedia();
        virtual ~SCXFilePersistMedia();
        virtual SCXHandle<SCXPersistDataReader> CreateReader(const std::wstring& name);
        virtual SCXHandle<SCXPersistDataWriter> CreateWriter(const std::wstring& name, unsigned int version = 0);
        virtual void UnPersist(const std::wstring& name);
        SCXFilePath NameToFilePath(const std::wstring& name) const;
        void SetBasePath(const SCXFilePath& path);
    private:
        void AddUserNameToBasePath();
        SCXFilePath m_BasePath; //!< Folder where persistence files are stored.
    };
}

#endif /* SCXFILEPERSISTMEDIA_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
