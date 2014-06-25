/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief     Platform independent filesystem interface

   \date      2007-08-24 12:34:56


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfilesystem.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxoserror.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxoserror.h>
#include <errno.h>
#include <iostream>

#if defined(SCX_UNIX)
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#endif

#include <sys/stat.h>


using namespace std;

namespace SCXCoreLib {

    /*-----------------------------------------------------------*/
    /** Represents the fully qualified path of the directory or file.

    \returns An absolute path

    If the info object was initially constructed using a relative path
    the full path was created relative the working directory at the time
    of construction
    */
    const SCXFilePath& SCXFileSystemInfo::GetFullPath() const {
        return m_fullPath;
    }

    /*-----------------------------------------------------------*/

    /** The path originally specified by the user, whether relative or absolute
        \returns An absolute or relative path
    */
    const SCXFilePath& SCXFileSystemInfo::GetOriginalPath() const {
        return m_originalPath;
    }

    /*--------------------------------------------------------------*/
    /**
           Get the attributes

           \returns  The attributes

    */
    const SCXFileSystem::Attributes SCXFileSystemInfo::GetAttributes() const {
        return m_attributes;
    }

    /*--------------------------------------------------------------*/
    /**
           Set the attributes

           \param[in]  attributes    Attributes to set

    */
    void SCXFileSystemInfo::SetAttributes(const SCXFileSystem::Attributes& attributes) {
        SCXFileSystem::SetAttributes(GetFullPath(), attributes);
        m_attributes = attributes;
    }

    /*--------------------------------------------------------------*/
    /**
           Get last access time as UTC.

           \returns The time the file system object was last accessed.
    */
    const SCXCalendarTime& SCXFileSystemInfo::GetLastAccessTimeUTC() const
    {
        return m_timeAccess;
    }

    /*--------------------------------------------------------------*/
    /**
           Get last modification time as UTC.

           \returns The time the file system object was last modified.
    */
    const SCXCalendarTime& SCXFileSystemInfo::GetLastModificationTimeUTC() const
    {
        return m_timeModification;
    }

    /*--------------------------------------------------------------*/
    /**
           Get last status change time as UTC.

           \returns The time the file system object had its status last changed.
    */
    const SCXCalendarTime& SCXFileSystemInfo::GetLastStatusChangeTimeUTC() const
    {
        return m_timeStatusChange;
    }

    /*--------------------------------------------------------------*/
    /**
           Get link count.

           \returns The link count.
    */
    scxulong SCXFileSystemInfo::GetLinkCount() const
    {
        return m_linkCount;
    }

    /*--------------------------------------------------------------*/
    /**
           Get size.

           \returns The size in bytes.
    */
    scxulong SCXFileSystemInfo::GetSize() const
    {
        return m_size;
    }

    /*--------------------------------------------------------------*/
    /**
           Get block size.

           \returns The block size in bytes.
    */
    scxulong SCXFileSystemInfo::GetBlockSize() const
    {
        return m_blockSize;
    }

    /*--------------------------------------------------------------*/
    /**
           Get block count.

           \returns The block count.
    */
    scxulong SCXFileSystemInfo::GetBlockCount() const
    {
        return m_blockCount;
    }

    /*--------------------------------------------------------------*/
    /**
           Get owning user ID.

           \returns The owning user id.
    */
    SCXUserID SCXFileSystemInfo::GetUserID() const
    {
        return m_uid;
    }

    /*--------------------------------------------------------------*/
    /**
           Get owning group ID.

           \returns The owning group ID.
    */
    SCXGroupID SCXFileSystemInfo::GetGroupID() const
    {
        return m_gid;
    }

    /*--------------------------------------------------------------*/
    /**
           Get device.

           \returns The device.
    */
    scxulong SCXFileSystemInfo::GetDevice() const
    {
        return m_device;
    }

    /*--------------------------------------------------------------*/
    /**
           Get device number.

           \returns The device number.

           \note only valid if file is a device.
    */
    scxulong SCXFileSystemInfo::GetDeviceNumber() const
    {
        return m_deviceNumber;
    }

    /*--------------------------------------------------------------*/
    /**
           Get serial number.

           \returns The serial number.
    */
    scxulong SCXFileSystemInfo::GetSerialNumber() const
    {
        return m_serialNumber;
    }

    /*--------------------------------------------------------------*/
    /**
           Check if path exists

           \returns  true if it exists

    */
    bool SCXFileSystemInfo::PathExists() const {
        return m_pathExists;
    }

    /*-----------------------------------------------------------*/

    /**
       Constructs an instance out of already known information.

       \param[in]      fullPath        Fully qualified path to the file
       \param[in]      originalPath    Either the fullPath or a part that was resolved to the fullPath
       \param[in]      pStat           A stat struct to use when initializing. If zero (default) a new one will be used.

       Useful when information about a file has already been received as part
       of another operation, and we would like to avoid unnecessary system calls.

       \note If the stat struct is given it must be filled  with data.
    */
    SCXFileSystemInfo::SCXFileSystemInfo(const SCXFilePath &fullPath,
                                         const SCXFilePath &originalPath,
                                         SCXFileSystem::SCXStatStruct* pStat /* = 0 */)
        : m_fullPath(fullPath), m_originalPath(originalPath), m_pathExists(false),
          m_linkCount(0), m_size(0), m_blockSize(0), m_blockCount(0),
          m_uid(0), m_gid(0), m_device(0), m_deviceNumber(0), m_serialNumber(0)
    {

        ReadFromDisk(pStat);

    }

    /*-----------------------------------------------------------*/

    const wstring SCXFileSystemInfo::DumpString() const {
        return m_originalPath.Get() + L" -> " + m_fullPath.Get();
    }

    /*--------------------------------------------------------------*/
    /**
        Reads information about the file from disk and updates the state

        \param[in] pStat Pointer to a pstat structure that should be used.
                         If zero (default) a new structure is fetched from disk.

        Helper method to retrive information from disk. If there is some problems with the path the
        exception to be thrown depends on the circumstances. If the path was received as a parameter
        we should throw InvalidArgumentException, but if the path was already part of the state we
        should throw InternalErrorException instead. The pathParam is used to decide what to do,
        if it is a nonempty string that is interpreted as if the path was received as a parameter.

        \note If the stat struct is given it must be filled  with data.
     */
    void SCXFileSystemInfo::ReadFromDisk(SCXFileSystem::SCXStatStruct* pStat /*= 0*/) {
        SCXFileSystem::SCXStatStruct statData;
        SCXFileSystem::SCXStatStruct* ps = pStat;
        try {
            if (0 == ps) {
                ps = &statData;
                SCXFileSystem::Stat(m_fullPath, ps);
            }
            m_attributes = SCXFileSystem::GetAttributes(ps);
            m_pathExists = true;
#if defined(linux) || defined(sun)
            m_timeAccess = SCXCalendarTime::FromPosixTime(ps->st_atim.tv_sec);
            m_timeAccess.SetSecond(m_timeAccess.GetSecond() + static_cast<double>(ps->st_atim.tv_nsec)/1000.0/1000.0/1000.0);
            m_timeModification = SCXCalendarTime::FromPosixTime(ps->st_mtim.tv_sec);
            m_timeModification.SetSecond(m_timeModification.GetSecond() + static_cast<double>(ps->st_mtim.tv_nsec)/1000.0/1000.0/1000.0);
            m_timeStatusChange = SCXCalendarTime::FromPosixTime(ps->st_ctim.tv_sec);
            m_timeStatusChange.SetSecond(m_timeStatusChange.GetSecond() + static_cast<double>(ps->st_ctim.tv_nsec)/1000.0/1000.0/1000.0);
#elif defined(aix)
            m_timeAccess = SCXCalendarTime::FromPosixTime(ps->st_atime);
            m_timeAccess.SetSecond(m_timeAccess.GetSecond() + ps->st_atime_n/1000.0/1000.0/1000.0);
            m_timeModification = SCXCalendarTime::FromPosixTime(ps->st_mtime);
            m_timeModification.SetSecond(m_timeModification.GetSecond() + ps->st_mtime_n/1000.0/1000.0/1000.0);
            m_timeStatusChange = SCXCalendarTime::FromPosixTime(ps->st_ctime);
            m_timeStatusChange.SetSecond(m_timeStatusChange.GetSecond() + ps->st_ctime_n/1000.0/1000.0/1000.0);
#elif defined(hpux)
            m_timeAccess = SCXCalendarTime::FromPosixTime(ps->st_atime);
            m_timeModification = SCXCalendarTime::FromPosixTime(ps->st_mtime);
            m_timeStatusChange = SCXCalendarTime::FromPosixTime(ps->st_ctime);
#if (PF_MAJOR==11) && (PF_MINOR>23)
        // Recent versions of HPUX has nano-second resolution on the timed
        // file attributes, so let's use it.
            m_timeAccess.SetSecond(m_timeAccess.GetSecond() + ps->st_natime/1000.0/1000.0/1000.0);
            m_timeModification.SetSecond(m_timeModification.GetSecond() + ps->st_nmtime/1000.0/1000.0/1000.0);
            m_timeStatusChange.SetSecond(m_timeStatusChange.GetSecond() + ps->st_nctime/1000.0/1000.0/1000.0);
#endif /* PF_MAJOR etc */
#elif defined(macos)
            m_timeAccess = SCXCalendarTime::FromPosixTime(ps->st_atimespec.tv_sec);
            m_timeAccess.SetSecond(m_timeAccess.GetSecond() + ps->st_atimespec.tv_nsec/1000.0/1000.0/1000.0);
            m_timeModification = SCXCalendarTime::FromPosixTime(ps->st_mtimespec.tv_sec);
            m_timeModification.SetSecond(m_timeModification.GetSecond() + ps->st_mtimespec.tv_nsec/1000.0/1000.0/1000.0);
            m_timeStatusChange = SCXCalendarTime::FromPosixTime(ps->st_ctimespec.tv_sec);
            m_timeStatusChange.SetSecond(m_timeStatusChange.GetSecond() + ps->st_ctimespec.tv_nsec/1000.0/1000.0/1000.0);
#elif defined(WIN32)
            m_timeAccess = SCXCalendarTime::FromPosixTime(ps->st_atime);
            m_timeModification = SCXCalendarTime::FromPosixTime(ps->st_mtime);
            //m_timeStatusChange = SCXCalendarTime::FromPosixTime(ps->st_ctime); // st_ctime is "create time"
#else
#error "Platform not supported"
#endif
            m_linkCount = ps->st_nlink;
            m_size = ps->st_size;
#if defined(SCX_UNIX)
            m_blockSize = ps->st_blksize;
            m_blockCount = ps->st_blocks;
#endif
            m_uid = ps->st_uid;
            m_gid = ps->st_gid;
            m_device = ps->st_dev;
            m_deviceNumber = ps->st_rdev;
            m_serialNumber = ps->st_ino;
        } catch (SCXFilePathNotFoundException&) {
            m_pathExists = false;
            m_attributes.clear();
        }
    }


    /*-----------------------------------------------------------*/
    /**
       (Re-)initializes path exists.
       \param[in] exists Value to initialize to.

    */
    void SCXFileSystemInfo::InitializePathExists(bool exists) {
        m_pathExists = exists;
    }

    /*-----------------------------------------------------------*/

    /**
       Creates a full (absolute) path out of a (possible) relative

       \param[in]  path    The path that we would like to have a fully qualified
       \returns    A fully qualified path

       If the path already is fully qualified it is returned as is, otherwise it is
       interpreted relative the current working directory to form a full path
    */
    SCXFilePath SCXFileSystem::CreateFullPath(const SCXFilePath& path) {
#if defined(WIN32)
        wstring absPath;
        wchar_t* absPathBuf = _wfullpath(NULL, path.Get().c_str(), 0);
        if (absPathBuf != NULL) {
            absPath = absPathBuf;
            free(absPathBuf);
        } else if (errno == ENOMEM) {
            throw SCXResourceExhaustedException(L"memory", L"Could not resolve full path", SCXSRCLOCATION);
        } else {
            throw SCXInternalErrorException(L"Unexpected errno " + StrFrom(errno), SCXSRCLOCATION);
        }
        return SCXFilePath(absPath);
#elif defined(SCX_UNIX)
        SCXFilePath fullPath;
        wstring filePath = path.GetDirectory();
        bool fileIsAbsolute = filePath.compare(0, 1, L"/") == 0 || filePath.compare(0, 1, L"\\") == 0
            || (filePath.length() > 3 && filePath.at(1) == ':' && filePath.at(2) == '\\');
        if (!fileIsAbsolute) {
            fullPath.SetDirectory(GetCurrentDirectory().Get());
        }
        fullPath.Append(filePath); // This is the full path, but it will possibly contain ".." and ".".

        std::vector<wstring> filePathTokens;
        StrTokenize(fullPath, filePathTokens,
                    wstring(1, SCXFilePath::GetFolderSeparator()), false);

        // Now we go through the tokens of the path and resolve the path by
        // pushing the correct tokens onto the resolvedFilePathTokens vector.
        std::vector<wstring> resolvedFilePathTokens;
        for (std::vector<wstring>::const_iterator token = filePathTokens.begin();
             token != filePathTokens.end();
             ++token) {
            if (*token == L".") {
                // skip.
            } else if (*token == L"..") {
                if (resolvedFilePathTokens.size() > 0) {
                    resolvedFilePathTokens.pop_back();
                }
            } else {
                resolvedFilePathTokens.push_back(*token);
            }
        }

        // Now generate a FilePath out of the vector.
        SCXFilePath resolvedPath(L"/");
        for (std::vector<wstring>::const_iterator token = resolvedFilePathTokens.begin();
             token != resolvedFilePathTokens.end();
             ++token) {
            resolvedPath.AppendDirectory(*token);
        }
        resolvedPath.SetFilename(path.GetFilename());
        return resolvedPath;
#else
#error
#endif
    }

    // on windows this symbol is defined as "GetCurrentDirectoryA/W" so compiler complains like
    // scxfilesystem.cpp(431) : error C2039: 'GetCurrentDirectoryW' : is not a member of 'SCXCoreLib::SCXFileSystem'

#undef GetCurrentDirectory
    /*-----------------------------------------------------------*/

    /**
       Gets the current working directory of the application.

       \returns The path of the current working directory.

       The current directory is distinct from the original directory,
       which is the one from which the process was started.
    */
    SCXFilePath SCXFileSystem::GetCurrentDirectory() {
#if defined(WIN32)
        wstring currentDirectory;
        wchar_t* currentDirectoryBuf = _wgetcwd (NULL, 0);
        if (currentDirectoryBuf != NULL) {
            currentDirectory = currentDirectoryBuf;
            free(currentDirectoryBuf);
        } else if (errno == ENOMEM) {
            throw SCXResourceExhaustedException(L"memory", L"Could not resolve full path", SCXSRCLOCATION);
        } else {
            throw SCXInternalErrorException(L"Unexpected errno " + StrFrom(errno), SCXSRCLOCATION);
        }
        return SCXFilePath(currentDirectory);
#elif defined(SCX_UNIX)
        string currentDirectory;
        long bufSize = pathconf(".", _PC_PATH_MAX);
        if (bufSize < 0) {
            throw SCXInternalErrorException(L"Unexpected errno " + StrFrom(errno), SCXSRCLOCATION);
        } else {
            char* currentDirectoryBuf = getcwd (NULL, bufSize);
            if (currentDirectoryBuf != NULL) {
                currentDirectory = currentDirectoryBuf;
                free(currentDirectoryBuf);
            } else if (errno == EACCES) {
                throw SCXUnauthorizedFileSystemAccessException(SCXFilePath(),
                                                               SCXFileSystem::Attributes(), SCXSRCLOCATION);
            } else {
                throw SCXInternalErrorException(L"Unexpected errno " + StrFrom(errno), SCXSRCLOCATION);
            }
        }
        return SCXFilePath(DecodePath(currentDirectory));
#else
#error
#endif
    }


    /*-----------------------------------------------------------*/
    /**
     * Retrieves the new line symbol to use in filesystem operation when
     * no information says otherwise.
     * \returns New line symbol to use
     */
    SCXStream::NLF SCXFileSystem::GetDefaultNewLine() {
#if defined(WIN32)
        return SCXStream::eCRLF;
#elif defined(SCX_UNIX)
        return SCXStream::eLF;
#else
#error
#endif
    }

    /*-----------------------------------------------------------*/
    /**
       Convert file pathes to 8-bit according to the system locale

       \param[in]      path           Path that should be converterd
       \returns        Path as "narrow" string

       There are no functions in glibc handling wide character filenames,
       this function converts to an 8-bit representation. The implementation
       is temporary. Currently it assumes UTF8 encoding.
    */
    string SCXFileSystem::EncodePath(const SCXFilePath& path) {
        return StrToUTF8(path.Get());
    }

    /*-----------------------------------------------------------*/
    /**
       Convert file pathes from 8-bit according to the system locale

       \param[in]      path           Path that should be converterd
       \returns        Path as "wide" string

       There are no functions in glibc handling wide character f�lenames,
       this function converts from an 8-bit representation. The implementation
       is temporary. Currently it assumes UTF8 encoding.
    */
    SCXFilePath SCXFileSystem::DecodePath(const string& path) {
        return SCXFilePath(StrFromUTF8(path));
    }

    /*-----------------------------------------------------------*/
    /**
       Creates a textual representation for a file attribute

       \param[in]  attribute   File attribute that we would like to represent as text
       \returns    Textual representation
    */
    wstring SCXFileSystem::AsText(SCXFileSystem::Attribute attribute) {
        wstring text;
        switch (attribute) {
        case eReadable:
            text = L"Readable";
            break;
        case eWritable:
            text = L"Writable";
            break;
        case eDirectory:
            text = L"Directory";
            break;
        case eUserRead:
            text = L"UserRead";
            break;
        case eUserWrite:
            text = L"UserWrite";
            break;
        case eUserExecute:
            text = L"UserExecute";
            break;
        case eGroupRead:
            text = L"GroupRead";
            break;
        case eGroupWrite:
            text = L"GroupWrite";
            break;
        case eGroupExecute:
            text = L"GroupExecute";
            break;
        case eOtherRead:
            text = L"OtherRead";
            break;
        case eOtherWrite:
            text = L"OtherWrite";
            break;
        case eOtherExecute:
            text = L"OtherExecute";
            break;
        case eUnknown:
        default:
            throw SCXInvalidArgumentException(L"attribute",
                                              L"Unknown SCXFileSystem::Attribute " + StrFrom(attribute), SCXSRCLOCATION);
        }
        return text;
    }

    /*-----------------------------------------------------------*/
    /**
       Creates a textual representation for a set of file attributes

       \param[in]  attributes   File attributes that we would like to represent as text
       \returns    Textual representation
    */
    wstring SCXFileSystem::AsText(const SCXFileSystem::Attributes& attributes) {
        wostringstream buf;
        bool firstItem = true;
        for (set<SCXFileSystem::Attribute>::const_iterator i = attributes.begin(); i != attributes.end(); ++i) {
            if (!firstItem) {
                buf << L",";
            }
            buf << AsText(*i);
            firstItem = false;
        }
        return buf.str();
    }

    /*-----------------------------------------------------------*/
    /**
       Fill in a stat structure.

       \param[in] path path to file/directory to perform stat on.
       \param[out] pStat Pointer to a stat structure that should be filled.
       \throws SCXInvalidArgumentException If pStat is zero.
       \throws SCXFilePathNotFoundException if path is not found.
       \throws SCXUnauthorizedFileSystemAccessException
    */
    void SCXFileSystem::Stat(const SCXFilePath& path, SCXStatStruct* pStat)
    {
        if (0 == pStat)
        {
            throw SCXInvalidArgumentException(L"pstat", L"Argument is NULL", SCXSRCLOCATION);
        }
#if defined(WIN32)
        wstring mangledPath = path.Get();
        // When doing stat on a directory on Windows, you may not have a trailing
        // '\' except when stating for example C:\ in which case you need it.
        if (mangledPath.size() != wstring(L"C:\\").size() ||
            mangledPath[1] != L':' ||
            mangledPath[2] != L'\\')
        {
            mangledPath = StrStripR(mangledPath, wstring(1, SCXFilePath::GetFolderSeparator()));
        }
        int failure = _wstat64(mangledPath.c_str(), pStat);
#elif defined(SCX_UNIX)
        std::string localizedName = SCXFileSystem::EncodePath(path);
        int failure = Stat(localizedName.c_str(), pStat );
#else
#error
#endif
        if (failure) {
            if (errno == ENOENT || errno == ENOTDIR) {
                throw SCXFilePathNotFoundException(path, SCXSRCLOCATION);
            } else if (errno == EACCES) {
                SCXFileSystem::Attributes attr;
                attr.insert(SCXFileSystem::eUnknown);
                throw SCXUnauthorizedFileSystemAccessException(path, attr, SCXSRCLOCATION);
            } else {
                throw SCXInvalidArgumentException(path.Get(), L"Failed to stat file or directory", SCXSRCLOCATION);
            }
        }
    }

    /*--------------------------------------------------------------*/
    /**
        Thin wrapper around the stat systemcall.
        \param path Path to use.
        \param pStat A stat structure.
        \returns Zero on success, otherwise false.
    */
    int SCXFileSystem::Stat(const char* path, SCXStatStruct* pStat)
    {
#if defined(macos) && (PF_MAJOR==10) && (PF_MINOR==4)
        return stat(path, pStat);
#elif defined(sun) && defined(BROKEN_TEST_FIXED)
        /*
          THIS CODE IS DISABLED FOR NOW - CAUSES UNIT TESTS TO FAIL!
          UNTIL THE BELOW SEGMENT IS FIXED, BETTER TO HAVE A TIMEOUT
          (AND CORRECT BEHAVIOR).  ULGY IN ALL CAPS, ISN'T THIS?
         */
        int r = lstat64(path, pStat);
        if (r >= 0 && S_ISLNK(pStat->st_mode))
        {
            // On Solaris, open or stat in the /device emulated directory with an
            // absent path or file name segment that has valid name syntax causes Solaris
            // to try to load a driver for the named segment.  This code avoids
            // the 15-second timeout when the device cannot be found by checking
            // each directory to see if it has an entry for the next segment berlow it.
            // This code is only run for symbolic links for efficiency reasons;
            // generally, the /device directory is only accessed through symbolic
            // links in the /dev directory.
            char fileName[FILENAME_MAX + 1];
            int fileNameSize = (int)readlink(path, fileName, FILENAME_MAX);
            if (fileNameSize < 0)
            {
                return -1;
            }

            fileName[fileNameSize] = '\0';  // readlink does not NUL-terminate--do so here
            if (fileName[0] != '/')
            {                           // handle relative file names
                char* slash = strrchr(const_cast<char*> (path), '/');
                if (slash != NULL)
                {
                    size_t pathNameSize = (size_t)(slash - path + 1);
                    if (pathNameSize + fileNameSize < FILENAME_MAX)
                    {
                        memmove(&fileName[pathNameSize], fileName, fileNameSize);
                        fileName[pathNameSize + fileNameSize + 1] = '\0';
                        memcpy(fileName, path, pathNameSize);
                    }
                }
            }

            // loop through path name segments and check each segment for the
            // presence of the name of the next segment in its directory
            char* dirEnd;
            char* fileBegin;
            char* fileEnd;
            dirent* entry;

            for (dirEnd = strchr(fileName + 1, '/'); dirEnd != NULL; dirEnd = fileEnd)
            {
                // open the directory portion of a file name
                *dirEnd = '\0';         // end the directory string at the separator
                DIR* dirp = opendir(fileName);
                if (dirp == NULL)
                {
                    // SCXDirectoryEnumerator::FindFiles errors if any file present
                    // in a directory errors on stat.  So, we return the symbolic link's
                    // information in pStat if any segment of its target is absent
                    return 0;
                }

                // get the name within the directory
                fileBegin = dirEnd + 1;
                fileEnd = strchr(fileBegin, '/');
                if (fileEnd != NULL)    // end the file name string at the separator
                {
                    *fileEnd = '\0';
                }

                // loop through the directory, seeking a matching file name
                while ((entry = readdir(dirp)) != NULL)
                {
                    if (strcmp(entry->d_name, fileBegin) == 0)
                    {
                        break;
                    }
                }
                closedir(dirp);

                // check to see if this name segment was present
                if (entry == NULL)
                {
                    // SCXDirectoryEnumerator::FindFiles errors if any file present
                    // in a directory errors on stat.  So, we return the symbolic link's
                    // information in pStat if any segment of its target is absent
                    return 0;
                }

                // put back the separators
                if (fileEnd != NULL)
                {
                    *fileEnd = '/';
                }
                *dirEnd = '/';
            }
            r = stat64(path, pStat);
        }
        return r;
#elif defined(SCX_UNIX)
        return stat64(path, pStat);
#elif defined(WIN32)
        return _stat64(path, pStat);
#else
#error "Platform not implemented"
#endif
    }

    /*--------------------------------------------------------------*/
    /**
        Gets the FileSystemAttributes of the file or directory on the path.

        \param[in]  path    The path to the file or directory.
        \returns            The Attributes of the file or directory on the path.
        \throws     SCXFilePathNotFoundException
        \throws     SCXInvalidArgumentException

        The path parameter is permitted to specify relative or absolute path information. Relative
        path information is interpreted as relative to the current working directory.
        To obtain the current working directory, see GetCurrentDirectory
    */
    SCXFileSystem::Attributes SCXFileSystem::GetAttributes(const SCXFilePath& path) {
        SCXFileSystem::Attributes attributes;
        SCXStatStruct buf;
        SCXFileSystem::Stat(path, &buf);
        attributes = SCXFileSystem::GetAttributes(&buf);
        return attributes;
    }

    /*--------------------------------------------------------------*/
    /**
        Gets the FileSystemAttributes of the given stat structure.

        \param[in]  pStat   The stat structure to get attributes from.
        \returns            The Attributes of the file or directory on the path.
        \throws     SCXInvalidArgumentException if argument is zero.

    */
    SCXFileSystem::Attributes SCXFileSystem::GetAttributes(SCXStatStruct* pStat)
    {
        SCXFileSystem::Attributes attributes;
        if (0 == pStat)
        {
            throw SCXInvalidArgumentException(L"pstat", L"Argument is NULL", SCXSRCLOCATION);
        }
#if defined(S_ISDIR)
        if (S_ISDIR(pStat->st_mode)) { // Do not check the directory bit alone since some sys files have the directory bit set too.
#else
        if (pStat->st_mode & _S_IFDIR) {
#endif
            attributes.insert(SCXFileSystem::eDirectory);
        }
        if (pStat->st_mode & S_IREAD) {
            attributes.insert(SCXFileSystem::eReadable);
        }
        if (pStat->st_mode & S_IWRITE) {
            attributes.insert(SCXFileSystem::eWritable);
        }
#if defined(SCX_UNIX)
        if (pStat->st_mode & S_IRUSR) {
            attributes.insert(SCXFileSystem::eUserRead);
        }
        if (pStat->st_mode & S_IWUSR) {
            attributes.insert(SCXFileSystem::eUserWrite);
        }
        if (pStat->st_mode & S_IXUSR) {
            attributes.insert(SCXFileSystem::eUserExecute);
        }
        if (pStat->st_mode & S_IRGRP) {
            attributes.insert(SCXFileSystem::eGroupRead);
        }
        if (pStat->st_mode & S_IWGRP) {
            attributes.insert(SCXFileSystem::eGroupWrite);
        }
        if (pStat->st_mode & S_IXGRP) {
            attributes.insert(SCXFileSystem::eGroupExecute);
        }
        if (pStat->st_mode & S_IROTH) {
            attributes.insert(SCXFileSystem::eOtherRead);
        }
        if (pStat->st_mode & S_IWOTH) {
            attributes.insert(SCXFileSystem::eOtherWrite);
        }
        if (pStat->st_mode & S_IXOTH) {
            attributes.insert(SCXFileSystem::eOtherExecute);
        }
#endif
        return attributes;
    }

    /*--------------------------------------------------------------*/
    /**
        Sets the specified FileAttributes of the file or directory on the specified path

        \param[in]  path            The path to the file or directory.
        \param[in]  attributes      The desired FileAttributes, such as Readable and Writable
        \throws     SCXUnauthorizedFileSystemAccessException    The caller does not have the required permission
        \throws     SCXFilePathNotFoundException                If no file or directory is found at path

        The path parameter is permitted to specify relative or absolute path information.
        Relative path information is interpreted as relative to the current working directory.
        To obtain the current working directory, see GetCurrentDirectory.

        It is not possible to change the compression status of a File object using the
        SetAttributes method

     */
    void SCXFileSystem::SetAttributes(const SCXFilePath& path, const SCXFileSystem::Attributes& attributes) {
        int permissions = 0;
        if (attributes.count(SCXFileSystem::eWritable) > 0) {
            permissions |= S_IWRITE;
        }
        if (attributes.count(SCXFileSystem::eReadable) > 0) {
            permissions |= S_IREAD;
        }
#if defined(SCX_UNIX)
        if (attributes.count(SCXFileSystem::eUserRead) > 0) {
            permissions |= S_IRUSR;
        }
        if (attributes.count(SCXFileSystem::eUserWrite) > 0) {
            permissions |= S_IWUSR;
        }
        if (attributes.count(SCXFileSystem::eUserExecute) > 0) {
            permissions |= S_IXUSR;
        }
        if (attributes.count(SCXFileSystem::eGroupRead) > 0) {
            permissions |= S_IRGRP;
        }
        if (attributes.count(SCXFileSystem::eGroupWrite) > 0) {
            permissions |= S_IWGRP;
        }
        if (attributes.count(SCXFileSystem::eGroupExecute) > 0) {
            permissions |= S_IXGRP;
        }
        if (attributes.count(SCXFileSystem::eOtherRead) > 0) {
            permissions |= S_IROTH;
        }
        if (attributes.count(SCXFileSystem::eOtherWrite) > 0) {
            permissions |= S_IWOTH;
        }
        if (attributes.count(SCXFileSystem::eOtherExecute) > 0) {
            permissions |= S_IXOTH;
        }
#endif
#if defined(WIN32)
        int failure = _wchmod(path.Get().c_str(), permissions);
#elif defined(SCX_UNIX)
        std::string localizedPath = SCXFileSystem::EncodePath(path);
        int failure = chmod(localizedPath.c_str(), permissions);
#else
#error
#endif

        if (failure) {
            switch (errno)
            {
            case EACCES:
            case EPERM:
            case EROFS:
                throw SCXUnauthorizedFileSystemAccessException(path, GetAttributes(path), SCXSRCLOCATION);
            case ENAMETOOLONG:
            case EINVAL:
                throw SCXInvalidArgumentException(L"path", path.Get(), SCXSRCLOCATION);
#if defined(SCX_UNIX)
            case ELOOP:
#endif
            case ENOENT:
            case ENOTDIR:
                throw SCXFilePathNotFoundException(path, SCXSRCLOCATION);
            default:
                wstring problem(L"Failed to set attributes for " + path.Get());
                throw SCXInternalErrorException(UnexpectedErrno(problem, errno), SCXSRCLOCATION);
            }
        }
    }

    /*--------------------------------------------------------------*/
    /**
        Moves a specified file or directory to a new location, providing the option to specify a
        new file name.

        \param[in]  oldPath     Existing file or directory to be moved
        \param[in]  newPath     New path, directory or file

        \throws     SCXUnauthorizedFileSystemAccessException    If oldPath specifies a directory and
                                                                newPath specifies a file
        \throws     SCXFilePathNotFoundException                If no file is found at oldPath

        If oldPath is a path to a file and newPath is a path not to a file but to an existing
        directory, the file is moved to that directory using the old name.

        This method works across disk volumes, and it does not throw an exception if the
        source and destination are the same. Note that if you attempt to replace a file by
        moving a file of the same name into that directory, you get an IOException. You
        cannot use the Move method to overwrite an existing file.

        The oldPath and newPath arguments are permitted to specify relative or
        absolute path information. Relative path information is interpreted as relative
        to the current working directory.

        \note The current implementation is restricted by the underlying API and requires newName to be complete
     */
    void SCXFileSystem::Move(const SCXFilePath& oldPath, const SCXFilePath &newPath) {
#if defined(WIN32)
        int failure = _wrename(oldPath.Get().c_str(), newPath.Get().c_str());
#elif defined(SCX_UNIX)
        std::string oldLocalizedPath = SCXFileSystem::EncodePath(oldPath);
        std::string newLocalizedPath = SCXFileSystem::EncodePath(newPath);
        int failure = rename(oldLocalizedPath.c_str(), newLocalizedPath.c_str());
#else
#error
#endif
        if (failure) {
            if (errno == EINVAL) {
                throw SCXInvalidArgumentException(L"path",
                    oldPath.Get() + L" or " + newPath.Get(), SCXSRCLOCATION);
            } else if (errno == EACCES || errno == EBUSY || errno == EROFS ||
                        errno == EISDIR || errno == EEXIST || errno == ENOTEMPTY ||
                        errno == ENOTDIR)  {
                throw SCXUnauthorizedFileSystemAccessException(newPath, SCXFileSystem::GetAttributes(newPath), SCXSRCLOCATION);
            } else if (errno == ENOENT) {
                throw SCXFilePathNotFoundException(oldPath, SCXSRCLOCATION);
            } else if (errno == EMLINK) {
                throw SCXFileSystemExhaustedException(L"directory entry", newPath, SCXSRCLOCATION);
            } else if (errno == ENOSPC) {
                throw SCXFileSystemExhaustedException(L"filesystem space", newPath, SCXSRCLOCATION);
            } else if (errno == EXDEV) {
                // May perhaps be implemented as copy and delete
                throw SCXNotSupportedException(L"Move files between file systems", SCXSRCLOCATION);
            } else {
                wstring problem(L"Failed to move " + oldPath.Get());
                throw SCXInternalErrorException(UnexpectedErrno(problem, errno), SCXSRCLOCATION);
            }
        }

    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
