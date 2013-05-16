/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Simple file based implementation of the SCXPersistMedia interface
    
    \date        2008-08-21 13:41:29

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxpersistence.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxuser.h>
#include "scxfilepersistmedia.h"
#include "scxfilepersistdatareader.h"
#include "scxfilepersistdatawriter.h"

namespace SCXCoreLib {

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.
    */
    SCXFilePersistMedia::SCXFilePersistMedia() :
        m_BasePath(L"/var/opt/microsoft/scx/lib/state/")
    {
        AddUserNameToBasePath();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor.
    */
    SCXFilePersistMedia::~SCXFilePersistMedia()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Create a new data reader and populate it with the data previously written
        with the given name.

        \param[in]  name     Name of data to populate reader with.
        \returns   The new reader.
        \throws an exception if no data previously written with the given name.
    */
    SCXHandle<SCXPersistDataReader> SCXFilePersistMedia::CreateReader(const std::wstring& name)
    {
        try
        {
            return SCXHandle<SCXPersistDataReader>(
                new SCXFilePersistDataReader(NameToFilePath(name)) );
        }
        catch (const SCXFilePathNotFoundException&)
        {
            throw PersistDataNotFoundException(name, SCXSRCLOCATION);
        }
        catch (const SCXUnauthorizedFileSystemAccessException&)
        {
            throw PersistDataNotFoundException(name, SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Add name of current user to base path.
    */
    void SCXFilePersistMedia::AddUserNameToBasePath()
    {
        SCXUser user;
        if (!user.IsRoot())
        {
            m_BasePath.AppendDirectory(user.GetName());
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Create a new data writer to write data with the given name.
        If data has previously been written with the same name, that data will be
        overwriten.

        \param[in]  name     Name of data to write.
        \param[in]  version  Version of data to write.
        \returns   The new writer.
    */
    SCXHandle<SCXPersistDataWriter> SCXFilePersistMedia::CreateWriter(const std::wstring& name, unsigned int version /* = 0*/)
    {
        try
        {
            return SCXHandle<SCXPersistDataWriter>(
                new SCXFilePersistDataWriter(NameToFilePath(name), version) );
        }
        catch (const SCXFilePathNotFoundException& e1)
        {
            throw PersistMediaNotAvailable(e1.What(), SCXSRCLOCATION);
        }
        catch (const SCXUnauthorizedFileSystemAccessException& e2)
        {
            throw PersistDataNotFoundException(e2.What(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Remove persisted data with the given name.

        \param[in]  name     Name of data to remove.
        \throws  PersistDataNotFoundException if no data previously written with the given name.
    */
    void SCXFilePersistMedia::UnPersist(const std::wstring& name)
    {
        SCXFileInfo f(NameToFilePath(name));
        if ( ! f.Exists())
        {
            throw PersistDataNotFoundException(name, SCXSRCLOCATION);
        }
        f.Delete();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the SCXPersistMedia implementation.
    
        \returns     SCXFilePersistMedia handle
    */
    SCXHandle<SCXPersistMedia> GetPersistMedia()
    {
        return SCXHandle<SCXPersistMedia>(new SCXFilePersistMedia());
    }

    /*----------------------------------------------------------------------------*/
    /**
        Translate a persistence name into a complete file path.
    
        \param[in]   name Persistence name to translate.
        \returns     SCXFilePath representing the name supplied.
    */
    SCXFilePath SCXFilePersistMedia::NameToFilePath(const std::wstring& name) const
    {
        SCXFilePath p(m_BasePath);
        // Replace all '_' with "__"
        std::wstring newname = name;
        for (std::wstring::size_type pos = newname.find(L'_');
             pos != std::wstring::npos;
             pos = newname.find(L'_', pos+2))
        {
            newname.insert(pos, 1, L'_');
        }
        
        // Replace all '/' with "_s"
        for (std::wstring::size_type pos = newname.find(L'/');
             pos != std::wstring::npos;
             pos = newname.find(L'/', pos+2))
        {
            newname[pos] = L's';
            newname.insert(pos, 1, L'_');
        }

        p.Append(newname);
        return p;
    }

    /*----------------------------------------------------------------------------*/
    /**
        This method is currently primarily for testing purposes.
    
        \param[in]   path New base path to use for storing files.
    */
    void SCXFilePersistMedia::SetBasePath(const SCXFilePath& path)
    {
        m_BasePath = path;
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
