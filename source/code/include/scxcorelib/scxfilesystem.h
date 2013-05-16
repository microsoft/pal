/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

 */
/**
    \file

    \brief     Platform independent filesystem interface

    \date      2007-08-24 12:34:56


 */
/*----------------------------------------------------------------------------*/
#ifndef SCXFILESYSTEMINFO_H
#define SCXFILESYSTEMINFO_H


#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/scxuser.h>
#include <string>
#include <set>

#include <sys/stat.h>

namespace SCXCoreLib {

    /*----------------------------------------------------------------------------*/
    /**
        Provides static methods for file system general functionality
     */
    class SCXFileSystem {
    public:
        //! Properties of a filesystem item
        enum Attribute {
            eUnknown, 
            eDirectory,  eReadable, eWritable,
            eUserRead, eUserWrite, eUserExecute,
            eGroupRead, eGroupWrite, eGroupExecute,
            eOtherRead, eOtherWrite, eOtherExecute
        };

        //! Set of properties of a filesystem item
        typedef std::set<Attribute> Attributes;

#if defined(WIN32)
        typedef struct __stat64 SCXStatStruct; //!< Abstraction of platform stat structure.
#elif defined(macos) && (PF_MAJOR==10) && (PF_MINOR==4)
        typedef struct stat SCXStatStruct; //!< Abstraction of platform stat structure.
#else
        typedef struct stat64 SCXStatStruct; //!< Abstraction of platform stat structure.
#endif

        static SCXFilePath CreateFullPath(const SCXFilePath&);
        static SCXStream::NLF GetDefaultNewLine();
        static std::string EncodePath(const SCXFilePath& path);
        static SCXFilePath DecodePath(const std::string& path);
        static std::wstring AsText(Attribute attribute);
        static std::wstring AsText(const Attributes& attributes);
        static void SetAttributes(const SCXFilePath&, const SCXFileSystem::Attributes& attributes);
        static SCXFileSystem::Attributes GetAttributes(const SCXFilePath&);
        static void Move(const SCXFilePath& oldPath, const SCXFilePath &newPath);

        static void Stat(const SCXFilePath& path, SCXStatStruct* pStat);
        static int Stat(const char* path, SCXStatStruct* pStat);
        static SCXFileSystem::Attributes GetAttributes(SCXStatStruct* pStat);
    private:
        //! Private constructor since this class should not be instanciated
        SCXFileSystem();
        static SCXFilePath GetCurrentDirectory();
    };

    /*----------------------------------------------------------------------------*/
    /**
        Provides the base class for both FileInfo and DirectoryInfo objects.


        The FileSystemInfo class contains methods that are common to file and directory
        manipulation. A FileSystemInfo object can represent either a file or a directory,
        thus serving as the basis for FileInfo or DirectoryInfo objects. Use this base class
        when parsing a lot of files and directories.

        When constructed FileSystemInfo calls Refresh and returns the cached information
        on APIs to get attributes and so on. On subsequent calls, you must call Refresh
        to get the latest copy of the information.
     */
    class SCXFileSystemInfo {
    public:
        const SCXFilePath& GetFullPath() const;
        const SCXFilePath& GetOriginalPath() const;

        /*----------------------------------------------------------------------------*/
        /**
           Convenient method to check if a FileSystemInfo object is a directory.

           \returns True if the object have the eDirectory attribute.
        */
        virtual bool IsDirectory() const 
        { 
            return 0 != GetAttributes().count(SCXFileSystem::eDirectory);
        }

        /** Cached file attributes

            \returns Last read file attribute, update using Refresh
        */
        virtual const SCXFileSystem::Attributes GetAttributes() const;

        /** Replaces the cached file attributes as well as the original on disk

            \param[in]  attributes  New attributes
            \throws     SCXFilePathNotFoundException
            \throws     SCXUnauthorizedFileSystemAccessException    The caller does not have the required permission
                                                                    or path is a directory
        */
        virtual void SetAttributes(const SCXFileSystem::Attributes& attributes);

        virtual const SCXCalendarTime& GetLastAccessTimeUTC() const;
        virtual const SCXCalendarTime& GetLastModificationTimeUTC() const;
        virtual const SCXCalendarTime& GetLastStatusChangeTimeUTC() const;
        virtual scxulong GetLinkCount() const;
        virtual scxulong GetSize() const;
        virtual scxulong GetBlockSize() const;
        virtual scxulong GetBlockCount() const;
        virtual SCXUserID GetUserID() const;
        virtual SCXGroupID GetGroupID() const;
        virtual scxulong GetDevice() const;
        virtual scxulong GetDeviceNumber() const;
        virtual scxulong GetSerialNumber() const;
        
        /** Gets a value indicating whether the file or directory exists.

            \returns true iff the path existed the last time we checked, update using Refresh
        */
        virtual bool PathExists() const;

        /** Refreshes the state of the object

            Takes a snapshot of information about the file on the current file system.
        */
        virtual void Refresh() = 0;

        /** Deletes the corresponding item of the file system
         */
        virtual void Delete() = 0;

        /** Dump class content to string

                \returns  The dumped content
        */
        virtual const std::wstring DumpString() const;

        /** Default implementation of virtual destructor
         */
        virtual ~SCXFileSystemInfo() { }
    protected:
        SCXFileSystemInfo(const SCXFilePath &fullPath, const SCXFilePath &originalPath, SCXCoreLib::SCXFileSystem::SCXStatStruct* pStat = 0);
        void ReadFromDisk(SCXFileSystem::SCXStatStruct* pStat = 0);
        void InitializePathExists(bool exists);
    private:
        SCXFilePath m_fullPath;         ///< Fully qualified (absolute) path
        SCXFilePath m_originalPath;     ///< Initially (potentially relative) specified path
        SCXFileSystem::Attributes m_attributes;     ///< Last known properties of the file
        bool m_pathExists;                          ///< Was there any item in the file system
        SCXCalendarTime m_timeAccess;       ///< List access time (st_atim in stat structure).
        SCXCalendarTime m_timeModification; ///< Last modification time (st_mtim in stat structure).
        SCXCalendarTime m_timeStatusChange; ///< Last status change time (st_ctim in stat structure).
        scxulong m_linkCount;               ///< Link count (st_nlink in stat structure).
        scxulong m_size;                    ///< Size (st_size in stat structure).
        scxulong m_blockSize;               ///< Block size (st_blksize in stat structure).
        scxulong m_blockCount;              ///< Block count (st_blocks in stat structure).
        SCXUserID m_uid;                    ///< Owner's user ID (st_uid in stat structure).
        SCXGroupID m_gid;                   ///< Owner's group ID (st_gid in stat structure).
        scxulong m_device;                  ///< Device (st_dev in stat structure).
        scxulong m_deviceNumber;            ///< Device number (st_rdev in stat structure).
        scxulong m_serialNumber;            ///< Serial number (st_ino in stat structure).
    };

    /*----------------------------------------------------------------------------*/
    /**
        Base class for all file system exceptions that "normally" can occur.


        A path indicating where in the file system the exception occured must always be specified.
        Should there be multiple locations causing problems, one of them should be specified.
     */
    class SCXFileSystemException : public SCXException {
    public:
        /// Path that the exception concerns
        SCXFilePath GetPath() const { return m_path; };
    protected:
        /**
         * Constructor
         * \param[in]    path        Path that the exception concerns
         * \param[in]    location    Where the exception occured
         */
        SCXFileSystemException(const SCXFilePath& path, const SCXCodeLocation& location)
                : SCXException(location),m_path(path) { };
    private:
        SCXFilePath m_path;   ///< Path that the exception concerns
    };

    /*----------------------------------------------------------------------------*/
    /**
       Access is not granted to a certain path in the file system.
    */
    class SCXUnauthorizedFileSystemAccessException : public SCXFileSystemException {
    public:
        /**
          Constructor
          \param[in]    path             Path not granted access to
          \param[in]    attributes       Attributes of the filesystem item not granted acces to
          \param[in]    location         Where the exception occured
        */
        SCXUnauthorizedFileSystemAccessException(const SCXFilePath& path,
                    const SCXFileSystem::Attributes& attributes, const SCXCodeLocation& location)
                : SCXFileSystemException(path, location), m_attributes(attributes) { };

        std::wstring What() const {
            return L"Failed to access filesystem item " + GetPath().Get();
        };

        /** Get the attributes

                \returns  The attributes
        */
        const SCXFileSystem::Attributes& GetAttributes() const {return m_attributes; }
    private:
        SCXFileSystem::Attributes m_attributes;         ///< The attributes
    };

    /*----------------------------------------------------------------------------*/

    /**
        A certain path does not exist in the file system
     */
    class SCXFilePathNotFoundException : public SCXFileSystemException {
    public:
        /**
          Constructor
          \param[in]    path             Path that does not exist on the filesystem
          \param[in]    location         Where the exception occured
        */
        SCXFilePathNotFoundException(const SCXFilePath& path, const SCXCodeLocation& location)
                : SCXFileSystemException(path, location) { };

        std::wstring What() const {
            return L"No item found in the filesystem at " + GetPath().Get();
        };
    };

    /*----------------------------------------------------------------------------*/
    /**
        Could not allocate needed file descriptor(s) due to no one available
    */
    class SCXFileSystemExhaustedException : public SCXFileSystemException {
    public:
        /**
          Constructor
          \param[in]    resource         Name of exhausted resource
          \param[in]    path             Path that to the filesystem item that could not be created
          \param[in]    location         Where the exception occured
        */
        SCXFileSystemExhaustedException(const std::wstring& resource, const SCXFilePath& path,
                    const SCXCodeLocation& location)
                : SCXFileSystemException(path, location),m_resource(resource) { };

        std::wstring What() const {
            return L"Failed to create filesystem item " + GetPath().Get() + L" due to lack of " + m_resource;
        };
    private:
        std::wstring m_resource;   ///< Name of exhausted resource
    };

}


#endif   /* SCXFILESYSTEMINFO_H */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
