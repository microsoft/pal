/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file        scxdirectoryinfo.h

    \brief       Defines the public interface for the directory PAL.

    \date        07-08-28 14:31:57

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXDIRECTORYINFO_H
#define SCXDIRECTORYINFO_H

#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxfilesystem.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxhandle.h>
#include <vector>

// On windows CreateDirectory is defined to CreateDirectoryW.
#if defined(WIN32)
#undef CreateDirectory
#endif
         
namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Represents different kinds of search options for the directory search methods.
    */
    enum SCXDirectorySearchOption
    {
        eDirSearchOptionNone = 0x000,
        eDirSearchOptionDir  = 0x001,
        eDirSearchOptionFile = 0x002,
        eDirSearchOptionSys  = 0x004,
        eDirSearchOptionAll  = eDirSearchOptionDir|eDirSearchOptionFile|eDirSearchOptionSys
    }; 

    /*----------------------------------------------------------------------------*/
    /**
       Represents a set of search options for the directory search methods.
    */
    typedef scxulong SCXDirectorySearchOptions;

    /*----------------------------------------------------------------------------*/
    /**
        Represents a directory in the filesystem. It exposes metods for listing
        the contents thereof and is the place to place additional methods for
        directory manipulation, should the need arise.
        The directory is represented by its file path through the SCXFilePath
        class. This means that an instance don't need to correspond to an
        existing directory in the file system, but rather more a potential directory.

        This class is in part modelled on the DirectoryInfo class of Microsoft .NET.

        \internal
        This class will inherit from SCXFileSystemInfo when that class is finished.
    */

    class SCXDirectoryInfo : public SCXFileSystemInfo {

        private:
        /** The default constructor is not allowed. */
        SCXDirectoryInfo();             // Intentionally not implemented (never used)

        public:
        /**
            Initializes a new instance of the SCXDirectoryInfo class on the specified path.
            \param dirname Pathname of directory
            \param pStat A stat struct to use when initializing. If zero (default) a new one will be used.

            This constructor does not check if the directory exists.
        */
        SCXDirectoryInfo(const SCXFilePath& dirname, SCXCoreLib::SCXFileSystem::SCXStatStruct* pStat = 0) :
            SCXFileSystemInfo(SCXFileSystem::CreateFullPath(dirname), dirname, pStat)
    {};

        void Refresh();
        void Delete();
        void Delete(bool recursive);

        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileSystemInfo> > GetFileSystemInfos(const SCXDirectorySearchOptions options = SCXCoreLib::eDirSearchOptionAll);
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > GetFiles();
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > GetSysFiles();
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXDirectoryInfo> > GetDirectories();
    };


    /*----------------------------------------------------------------------------*/
    /**
     * Utility methods for managing directories. Useful when a single operation
     * is to be performed on a directory
     */
    class SCXDirectory {
    public:
        static void Delete(const SCXFilePath &path, bool recursive = false);
        static void Move(const SCXFilePath& oldPath, const SCXFilePath &newPath);
        static bool Exists(const SCXFilePath&);
        static SCXDirectoryInfo CreateDirectory(const SCXFilePath& path);
#if !defined(DISABLE_WIN_UNSUPPORTED)        
        static SCXDirectoryInfo CreateTempDirectory();
#endif

        static std::vector<SCXCoreLib::SCXFilePath> GetFileSystemEntries(const SCXFilePath &path, const SCXDirectorySearchOptions options = SCXCoreLib::eDirSearchOptionAll);
        static std::vector<SCXCoreLib::SCXFilePath> GetFiles(const SCXFilePath &path);
        static std::vector<SCXCoreLib::SCXFilePath> GetDirectories(const SCXFilePath &path);
        static std::vector<SCXCoreLib::SCXFilePath> GetSysFiles(const SCXFilePath &path);
    private:
        SCXDirectory(); //!< Utility class, prevent instances
    };

}
#endif /* SCXDIRECTORYINFO_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
