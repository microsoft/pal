/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file        scxdirectoryinfo.cpp

   \brief       Defines the public interface for the directory PAL.

   \date        07-08-28 15:28:40

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxoserror.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/strerror.h>

#include <string>
#include <vector>

#ifdef WIN32
#include <io.h>
#else /* WIN32 */
#include <dirent.h>
#include <libgen.h>
#endif /* WIN32 */

#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <iostream>

#if defined(aix) || defined(linux)
#include <unistd.h>
#endif

#if ! defined(WIN32)
/** _D_ALLOC_NAMLEN is a rather neat way to compute an upper bound of how many bytes
    of d_name to copy from the dirent structure. This macro is pre-defined on Linux.
*/
#ifndef _D_ALLOC_NAMLEN
# if defined(hpux) || defined(aix)
#  define _D_ALLOC_NAMLEN(d) ((d)->d_namlen + 1)
# elif defined(sun)
#  define _D_ALLOC_NAMLEN(d) ((d)->d_reclen - (&(d)->d_name[0] - (char*)(d)))
# else
#  define _D_ALLOC_NAMLEN(d) (nameendsz)
# endif
#endif

/* The PATH_MAX value may be omitted from limits.h and in that case the value is
   provided by pathconf(). We do the simpler solution an define it to be the minimum
   acceptable value.
*/
#ifndef PATH_MAX
#define PATH_MAX _XOPEN_PATH_MAX
#endif

#endif /* ! WIN32 */

namespace
{
    /*--------------------------------------------------------------*/
    /**
       Implements default behaviour for the SCXDirectoryEnumerator. 
       This is however a virtual class since the result method is purly virtual.
    */
    class SCXDirectoryEnumeratorBehaviour
    {
    public:
        /*--------------------------------------------------------------*/
        /**
           Virtual destructor.
        */
        virtual ~SCXDirectoryEnumeratorBehaviour() { }

        /*--------------------------------------------------------------*/
        /**
           Return a stat result.

           \param[in] path Path to file or directory to stat.
           \returns A pointer to the stat result or zero if stat fails.
        */
        virtual SCXCoreLib::SCXFileSystem::SCXStatStruct* DoStat(const char* path)
        {
            memset(&m_stat, 0, sizeof(m_stat));
            try
            {
                SCXCoreLib::SCXFileSystem::Stat(SCXCoreLib::SCXFileSystem::DecodePath(path), &m_stat);
            }
            catch (SCXCoreLib::SCXException&)
            {
                return 0;
            }
            return &m_stat;
        }

        /*--------------------------------------------------------------*/
        /**
           Update the internal stat structure.

           \param[in] path Path to file or directory to stat.
           \throws SCXErrnoException If the stat call fails.
        */
        virtual void UpdateStat(const SCXCoreLib::SCXFilePath& path)
        {
            
            if (0 == DoStat(SCXCoreLib::StrToUTF8(path.Get()).c_str()))
            {
                if (errno == ENOENT)
                { // Someone has removed the file before we got a chance to look at it. silently ignore
                    return;
                }
                throw SCXCoreLib::SCXErrnoException(L"stat64", errno, SCXSRCLOCATION);
            }
        }

        /*--------------------------------------------------------------*/
        /**
           Handle a result.

           \param[in] path Path to the found file or directory.
           \param[in] statCalled True if stat was called (and the internal stat structure 
                                 is up to date). Otherwise false.

           This method is called for each file or directory found that matches the search
           criterias.
        */
        virtual void Result(const SCXCoreLib::SCXFilePath& path, bool statCalled) = 0;
    protected:
        SCXCoreLib::SCXFileSystem::SCXStatStruct m_stat; //!< Internal stat structure with latest stat result.
    };

    /*--------------------------------------------------------------*/
    /**
       Implements directory enumerator behaviour when only path
       is desired.
    */
    class SCXDirectoryEnumeratorBehaviourPath : public SCXDirectoryEnumeratorBehaviour
    {
    public:
        /** \copydoc SCXDirectoryEnumeratorBehaviour::~SCXDirectoryEnumeratorBehaviour */
        virtual ~SCXDirectoryEnumeratorBehaviourPath() { }

        /** \copydoc SCXDirectoryEnumeratorBehaviour::Result */
        virtual void Result(const SCXCoreLib::SCXFilePath& path, bool /*statCalled*/)
        {
            m_files.push_back(path);
        }
        
        /*--------------------------------------------------------------*/
        /**
           Get the result.

           \returns A vector of all found paths.
        */
        std::vector<SCXCoreLib::SCXFilePath>& GetFiles()
        {
            return m_files;
        }
    private:
        //! The result vector.
        std::vector<SCXCoreLib::SCXFilePath> m_files;
    };

    /*--------------------------------------------------------------*/
    /**
       Implements directory enumerator behaviour when FileInfo
       is desired.
    */
    class SCXDirectoryEnumeratorBehaviourFileInfo : public SCXDirectoryEnumeratorBehaviour
    {
    public:
        /** \copydoc SCXDirectoryEnumeratorBehaviour::~SCXDirectoryEnumeratorBehaviour */
        virtual ~SCXDirectoryEnumeratorBehaviourFileInfo() { }

        /** \copydoc SCXDirectoryEnumeratorBehaviour::Result */
        virtual void Result(const SCXCoreLib::SCXFilePath& path, bool statCalled)
        {
            if ( ! statCalled)
            {
                UpdateStat(path);
            }

            SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> info( new SCXCoreLib::SCXFileInfo(path, &m_stat) );
            m_files.push_back(info);
        }

        /*--------------------------------------------------------------*/
        /**
           Get the result.

           \returns A vector of all found info objects.
        */
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> >& GetFiles()
        {
            return m_files;
        }
    private:
        //! The result vector.
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > m_files;
    };

    /*--------------------------------------------------------------*/
    /**
       Implements directory enumerator behaviour when DirectoryInfo
       is desired.
    */
    class SCXDirectoryEnumeratorBehaviourDirectoryInfo : public SCXDirectoryEnumeratorBehaviour
    {
    public:
        /** \copydoc SCXDirectoryEnumeratorBehaviour::~SCXDirectoryEnumeratorBehaviour */
        virtual ~SCXDirectoryEnumeratorBehaviourDirectoryInfo() { }

        /** \copydoc SCXDirectoryEnumeratorBehaviour::Result */
        virtual void Result(const SCXCoreLib::SCXFilePath& path, bool statCalled)
        {
            if ( ! statCalled)
            {
                UpdateStat(path);
            }

            SCXCoreLib::SCXHandle<SCXCoreLib::SCXDirectoryInfo> info( new SCXCoreLib::SCXDirectoryInfo(path, &m_stat) );
            m_files.push_back(info);
        }

        /*--------------------------------------------------------------*/
        /**
           Get the result.

           \returns A vector of all found info objects.
        */
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXDirectoryInfo> >& GetFiles()
        {
            return m_files;
        }
    private:
        //! The result vector.
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXDirectoryInfo> > m_files;
    };

    /*--------------------------------------------------------------*/
    /**
       Implements directory enumerator behaviour when FileSystemInfo
       is desired.
    */
    class SCXDirectoryEnumeratorBehaviourFileSystemInfo : public SCXDirectoryEnumeratorBehaviour
    {
    public:
        /** \copydoc SCXDirectoryEnumeratorBehaviour::~SCXDirectoryEnumeratorBehaviour */
        virtual ~SCXDirectoryEnumeratorBehaviourFileSystemInfo() { }

        /** \copydoc SCXDirectoryEnumeratorBehaviour::Result */
        virtual void Result(const SCXCoreLib::SCXFilePath& path, bool statCalled)
        {
            if ( ! statCalled)
            {
                UpdateStat(path);
            }
#if defined(S_ISDIR)
            if (S_ISDIR(m_stat.st_mode))
#else
            if (m_stat.st_mode & _S_IFDIR)
#endif
            {
                SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileSystemInfo> info( new SCXCoreLib::SCXDirectoryInfo(path, &m_stat) );
                m_files.push_back(info);
            }
            else 
            {
                SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileSystemInfo> info( new SCXCoreLib::SCXFileInfo(path, &m_stat) );
                m_files.push_back(info);
            }
        }

        
        /*--------------------------------------------------------------*/
        /**
           Get the result.

           \returns A vector of all found info objects.
        */
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileSystemInfo> >& GetFiles()
        {
            return m_files;
        }
    private:
        //! The result vector.
        std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileSystemInfo> > m_files;
    };

    /*--------------------------------------------------------------*/
    /**
       A helper class to enumerate directories.
    */
    class SCXDirectoryEnumerator
    {
    public:
        /*--------------------------------------------------------------*/
        /**
           Constructor.

           \param[in] deps A dependency object needed for details of enumeration pattern.

           The dependency object will determine what kind of result you get.
           The result is stored in the dependency object.
        */
        SCXDirectoryEnumerator(const SCXCoreLib::SCXHandle<SCXDirectoryEnumeratorBehaviour>& deps)
            : m_deps(deps)
        {
        }

        /*--------------------------------------------------------------*/
        /**
           Enumerate all file objects in a given directory.

           \param[in] path path to folder to enumerate.
           \param[in] options Search options i.e. directories, files and/or sys files.
           \throws SCXFilePathNotFoundException
           \throws SCXInvalidArgumentException
           \throws SCXResourceExhaustedException

           The result will be handled by the dependency object via a call to the Result
           method of the dependency object.
        */
        void FindFiles(const SCXCoreLib::SCXFilePath& path, const SCXCoreLib::SCXDirectorySearchOptions& options)
        {
#if defined(WIN32)
            struct _wfinddata_t c_file;     // Directory entry. From <wchar.h>
            intptr_t hFile;                 // Handle to directory. From <crtdefs>.h
            errno_t eno;                    // Temporary errno storage

            // Extract wide char representation of directory name from SCXFilePath and append a * wildcard
            std::wstring wdirname(path.GetDirectory());
            wdirname += L'*';

            // Open the directory and get the handle
            hFile = _wfindfirst(wdirname.c_str(), &c_file);
            if( hFile == -1L ) {
                // It failed. Read error code and make an exception of it.
                eno = errno;
                errno = 0;

                // Not finding the directory is an exception
                if (eno == ENOENT) { 
                    throw SCXCoreLib::SCXFilePathNotFoundException(path.GetDirectory(), SCXSRCLOCATION);
                }

                wchar_t errtxt[100];
                __wcserror_s(errtxt, sizeof errtxt / sizeof(wchar_t), wdirname.c_str());
                if (eno == EINVAL) {
                    // Unlike Unix, we get EINVAL even if the dir is read protected
                    throw SCXCoreLib::SCXInvalidArgumentException(wdirname, errtxt, SCXSRCLOCATION);
                } else {
                    throw SCXCoreLib::SCXResourceExhaustedException(errtxt, wdirname, SCXSRCLOCATION);
                }
            }
            do {
                // Undocumented feature: Junction Files/Reparse Points are silently skipped
                if ((c_file.attrib & 0x400)) { continue; }

                // Check file attributes and decide if this file should be included
                bool isSystem = ((c_file.attrib & _A_SYSTEM) != 0);

                // Case 1: This is a system file and those should not be listed at all
                if (isSystem && 0 == (options & SCXCoreLib::eDirSearchOptionSys)) { continue; }

                // Case 2: This is a subdir
                if (c_file.attrib & _A_SUBDIR) {

                    if (0 == (options & SCXCoreLib::eDirSearchOptionDir)) { continue;} // ...and those should not be listed
                    // Do away with . and ..
                    if (c_file.name[0] == L'.'
                        && (c_file.name[1] == L'\0'
                            || (c_file.name[1] == L'.' && c_file.name[2] == L'\0'))) { continue; }

                    SCXCoreLib::SCXFilePath f(path);         // Use current path and filename
                    f.SetFilename(L"");                     // Only interested in path component
                    f.AppendDirectory(std::wstring(c_file.name));
                    m_deps->Result(f, false);
                }
                else {
                    // Case 3: This an ordinay file
                    if (0 == (options & SCXCoreLib::eDirSearchOptionFile) && !isSystem) {continue;}// ... and those should not be listed unless being system files
                    SCXCoreLib::SCXFilePath f(path);         // Use current path and filename
                    f.SetFilename(std::wstring(c_file.name));
                    m_deps->Result(f, false);
                }

            } while( _wfindnext( hFile, &c_file ) == 0 );

            // Check errno but first release resources. ENOENT is ok (and expected).
            _get_errno(&eno);
            _findclose(hFile);
            if (eno != ENOENT && eno != 0) { 
                // If we get here something went wrong and an exception should be thrown
                wchar_t errtxt[100];
                __wcserror_s(errtxt, sizeof errtxt / sizeof(wchar_t), wdirname.c_str());
                if (eno == EINVAL) {
                    throw SCXCoreLib::SCXInvalidArgumentException(wdirname, errtxt, SCXSRCLOCATION);
                } else {
                    throw SCXCoreLib::SCXResourceExhaustedException(errtxt, wdirname, SCXSRCLOCATION);
                }
            }
#else /* WIN32 */
    /*
      Note: This implementation is supposed to be mostly POSIX 1003.1 compliant.
      Using the d_type field of the dirent structure is however not. Posix
      don't require that this field is present. Instead you do a stat() call
      on each file to figure out its file type. This means an extra
      file access for each file in the directory. Using d_type is ok on
      all BSD derived unixes such as Linux. On Solaris it isn't, and HP/UX is still unknown.

      Strictly speaking, readdir() is not thread safe. But that only applies
      if multiple threads access the same dirstream and that is not the case
      in this code.

      Right now the function is hardwired to use UTF-8 as external character set.
      I would like see a method like SCXFilePath::GetLocalEncoding() that returns
      a char* with the fully qualified pathname encoded in the local character set.
    */

            DIR *dirp = 0;                  // Directory stream object. From <dirent.h>
            struct dirent *dentp = 0;       // Pointer to a directory entry. From <dirent.h>
            int eno = 0;                    // Temporary storage for errno.
            char filenamebuf[PATH_MAX];     // Used to construct all filenames
            char *nameendptr = 0;           // Position after directory name
            size_t nameendsz = 0;           // Remaing space in filenamebuf
            size_t copysz = 0;              // How many characters to strncopy to filenamebuf

            errno = 0;

            // Extract "char *" representation of directory name from SCXFilePath.
            std::wstring wdirname(path.GetDirectory());
            std::string dirname(SCXCoreLib::StrToUTF8(wdirname)); // May throw, but that's ok?
            strncpy(filenamebuf, dirname.c_str(), sizeof(filenamebuf));
            // In the future I expect to be able to just do the following and be done with it
            // path.GetLocalEncoding(filenamebuf, sizeof(filenamebuf));

            // Prepare file name buffer for reuse (every name is temporarily stored here)
            nameendptr = filenamebuf + strlen(filenamebuf);
            // if (nameendptr[-1] != '/') { *nameendptr = '/'; ++nameendptr; *nameendptr = '\0'; }
            nameendsz = sizeof(filenamebuf) - (nameendptr - filenamebuf) - 2;

            // Open the directory and get the handle
            dirp = opendir(filenamebuf);
            if (dirp == 0) {
                // It failed. Read error code and make an exception of it.
                eno = errno;
                errno = 0;

                if (eno == ENOENT) { 
                    throw SCXCoreLib::SCXFilePathNotFoundException(path.GetDirectory(), SCXSRCLOCATION);
                }

                std::string errtxt(SCXCoreLib::strerror(eno));

                if (eno == EACCES || eno == ENOTDIR) {
                    throw SCXCoreLib::SCXInvalidArgumentException(wdirname, SCXCoreLib::StrFromUTF8(errtxt), SCXSRCLOCATION);
                } else {
                    // All other errno's are resource related. Memory, descriptors, etc.
                    // Note that the parameters supplied to the exception are not a very good
                    // match to what it expects.
                    throw SCXCoreLib::SCXResourceExhaustedException(SCXCoreLib::StrFromUTF8(errtxt), wdirname, SCXSRCLOCATION);
                }
            }

            // Read a directory entry
            struct dirent *currentDir = NULL;
            int len_entry = offsetof(struct dirent, d_name) + PATH_MAX + 1;
            currentDir = (struct dirent *)malloc(len_entry);
#if defined(sun)
            while (NULL != (dentp = readdir_r(dirp, currentDir))) {
#else
            while ((0 == readdir_r(dirp, currentDir, &dentp)) && NULL != dentp) {
#endif
                bool isdir = false;                         // Is this a directory entry
                SCXCoreLib::SCXFileSystem::SCXStatStruct* pStat = 0;
                // Only BSD-based C-libraries have the d_type entry in the DIR structure
                // In all others we must always do an explict stat call to find out the type
#if defined(linux)
                // Check file type and decide if this file should be included
                switch (dentp->d_type) {

                case DT_LNK:     /* Always dereference symlinks */
                case DT_UNKNOWN: /* File type can't be found out from dirent. Do a stat() call. */
#endif
                    /* Decide how many characters to copy. (We do this because the buffer is big)*/
                    copysz = MIN(nameendsz, static_cast<size_t>(_D_ALLOC_NAMLEN(dentp)));
                    strncpy(nameendptr, dentp->d_name, copysz);
                    
                    pStat = m_deps->DoStat(filenamebuf);
                    if (0 == pStat) {
                        /* We must account for links that point to non-existing files */
                        if (errno == ENOENT) { errno = 0; continue; }
                        else { goto after; }
                    }
                    if (S_ISREG(pStat->st_mode)) {
                        if (0 == (options & SCXCoreLib::eDirSearchOptionFile)) { continue; }
                        isdir = false;
                    } else if (S_ISDIR(pStat->st_mode)) {
                        if (0 == (options & SCXCoreLib::eDirSearchOptionDir)) { continue; }
                        isdir = true;
                    }
                    else {
                        if (0 == (options & SCXCoreLib::eDirSearchOptionSys)) { continue; }
                    }
#if defined(linux)
                    break;

                case DT_REG:
                    // This is a regular file
                    if (0 == (options & SCXCoreLib::eDirSearchOptionFile)) { continue; }
                    isdir = false;
                    break;

                case DT_DIR:
                    // This is a directory
                    if (0 == (options & SCXCoreLib::eDirSearchOptionDir)) { continue; }
                    isdir = true;
                    break;

                default:
                    // This is a "system file" (Device, socket, etc.)
                    if (0 == (options & SCXCoreLib::eDirSearchOptionSys)) { continue; }
                }
#endif
                if (isdir) {
                    // Do away with . and ..
                    if (dentp->d_name[0] == '.'
                        && (dentp->d_name[1] == '\0'
                            || (dentp->d_name[1] == '.' && dentp->d_name[2] == '\0'))) { continue;}
                }

                // make new SCXFilePath structure with current path and filename, add to result
                std::string dname(dentp->d_name);
                std::wstring wdname(SCXCoreLib::StrFromUTF8(dname));

                SCXCoreLib::SCXFilePath f(path);             // Use current path and filename
                if (isdir) {
                    f.SetFilename(L"");                     // Only interested in path component
                    f.AppendDirectory(wdname);
                } else { // Ordinary file or system
                    f.SetFilename(wdname);
                }
                m_deps->Result(f, pStat != 0);
            }

        after:
            if (currentDir)
            {
                free(currentDir);
            }
            eno = errno;
            errno = 0;
            closedir(dirp);
            if (eno != 0) { 
                throw SCXCoreLib::SCXErrnoException(L"stat", eno, SCXSRCLOCATION);
            }
#endif
        }

    private:
        SCXCoreLib::SCXHandle<SCXDirectoryEnumeratorBehaviour> m_deps; //!< Dependency object.
    };
}

namespace SCXCoreLib
{

    /*--------------------------------------------------------------*/
    /**
           Reread directory information

    */
    void SCXDirectoryInfo::Refresh() {
        ReadFromDisk();
    }

    /*--------------------------------------------------------------*/
    /**
           Delete the directory

    */
    void SCXDirectoryInfo::Delete() {
        SCXDirectory::Delete(GetFullPath());
        InitializePathExists(false);
    }

    /*--------------------------------------------------------------*/
    /**
       Delete the directory, and if indicated, the content recursively

       \param recursive If true the content of the directory is deleted too.
    */
    void SCXDirectoryInfo::Delete(bool recursive) {
        SCXDirectory::Delete(GetFullPath(), recursive);
        InitializePathExists(false);
    }

    /*--------------------------------------------------------------*/
    /**
       Get FileSystemInfos for objects in directory.

       \param[in] options What types of files (regular, system and/or directories).
                          Default is all types of objects.
       \returns A vector of SCXHandles to SCXFileSystemInfo objects for all file system
                objects in folder.
       \throws     SCXFilePathNotFoundException if path is not found.
    */
    std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileSystemInfo> > SCXDirectoryInfo::GetFileSystemInfos(const SCXDirectorySearchOptions options/* = SCXCoreLib::eDirSearchOptionAll*/)
    {
#if defined(WIN32) /* Windows is very picky about the template types - and hpux picky in the other direction */
        const 
#endif
        SCXCoreLib::SCXHandle<SCXDirectoryEnumeratorBehaviourFileSystemInfo> info( new SCXDirectoryEnumeratorBehaviourFileSystemInfo() );
        SCXDirectoryEnumerator enumerator(info);
        enumerator.FindFiles(GetFullPath(), options);
        return info->GetFiles();
    }

    /*--------------------------------------------------------------*/
    /**
       Get FileInfos for all regular files in directory.

       \returns A vector of SCXHandles to SCXFileInfo objects for all regular files in directory.
       \throws     SCXFilePathNotFoundException if path is not found.
    */
    std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > SCXDirectoryInfo::GetFiles()
    {
#if defined(WIN32) /* Windows is very picky about the template types - and hpux picky in the other direction */
        const 
#endif
        SCXCoreLib::SCXHandle<SCXDirectoryEnumeratorBehaviourFileInfo> info( new SCXDirectoryEnumeratorBehaviourFileInfo() );
        SCXDirectoryEnumerator enumerator(info);
        enumerator.FindFiles(GetFullPath(), SCXCoreLib::eDirSearchOptionFile);
        return info->GetFiles();
    }

    /*--------------------------------------------------------------*/
    /**
       Get FileInfos for all system files in directory.

       \returns A vector of SCXHandles to SCXFileInfo objects for all system files in directory.
       \throws     SCXFilePathNotFoundException if path is not found.
    */
    std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXFileInfo> > SCXDirectoryInfo::GetSysFiles()
    {
#if defined(WIN32) /* Windows is very picky about the template types - and hpux picky in the other direction */
        const 
#endif
        SCXCoreLib::SCXHandle<SCXDirectoryEnumeratorBehaviourFileInfo> info( new SCXDirectoryEnumeratorBehaviourFileInfo() );
        SCXDirectoryEnumerator enumerator(info);
        enumerator.FindFiles(GetFullPath(), SCXCoreLib::eDirSearchOptionSys);
        return info->GetFiles();
    }

    /*--------------------------------------------------------------*/
    /**
       Get DirectoryInfos for all directories in directory.

       \returns A vector of SCXHandles to SCXDirectoryInfo objects for all directories in directory.
       \throws     SCXFilePathNotFoundException if path is not found.
    */
    std::vector<SCXCoreLib::SCXHandle<SCXCoreLib::SCXDirectoryInfo> > SCXDirectoryInfo::GetDirectories()
    {
#if defined(WIN32) /* Windows is very picky about the template types - and hpux picky in the other direction */
        const 
#endif
        SCXCoreLib::SCXHandle<SCXDirectoryEnumeratorBehaviourDirectoryInfo> info( new SCXDirectoryEnumeratorBehaviourDirectoryInfo() );
        SCXDirectoryEnumerator enumerator(info);
        enumerator.FindFiles(GetFullPath(), SCXCoreLib::eDirSearchOptionDir);
        return info->GetFiles();
    }

    /*--------------------------------------------------------------*/
    /**
     * Delete directory and, if specified, its content
     * @param[in]   path        Path to directory
     * @param[in]   recursive   true iff content is to e deleted
     */
    void SCXDirectory::Delete(const SCXFilePath &path, bool recursive) {
        if (recursive) {
            std::vector<SCXFilePath> paths = GetDirectories(path);
            for (size_t nr = 0; nr < paths.size(); nr++) {
                SCXDirectory::Delete(paths[nr], recursive);
            }
            paths = GetFileSystemEntries(path, SCXCoreLib::eDirSearchOptionFile | SCXCoreLib::eDirSearchOptionSys);

            for (size_t nr = 0; nr < paths.size(); nr++) {
                SCXFile::Delete(paths[nr]);
            }
        }
#if defined(WIN32)
        int failure = _wrmdir(path.Get().c_str());
#elif defined(SCX_UNIX)
        std::string localizedPath = SCXFileSystem::EncodePath(path);
        int failure = rmdir(localizedPath.c_str());
#else
#error
#endif
        if (failure) {
            switch (errno) {
            case EACCES:
            case EBUSY:
            case EPERM:
            case EROFS:
            case ENOTDIR:
            case ENOTEMPTY:
#if defined(sun) || defined(hpux)
            case EEXIST:
#endif
                throw SCXUnauthorizedFileSystemAccessException(path, SCXFileSystem::GetAttributes(path), SCXSRCLOCATION);
            case EINVAL:
#if defined(SCX_UNIX)
            case ELOOP:
#endif
            case ENAMETOOLONG:
                throw SCXInvalidArgumentException(L"path", L"Invalid format " + path.Get(), SCXSRCLOCATION);
            case ENOENT:
                // Job already done!
                break;
            default:
                std::wstring problem(L"Failed to delete " + path.Get());
                throw SCXInternalErrorException(UnexpectedErrno(problem, errno), SCXSRCLOCATION);
            }
        }

    }

    /*--------------------------------------------------------------*/
    /**
        Moves a specified directory to a new location

        \param[in]  oldPath     Existing directory to be moved
        \param[in]  newPath     New directory path

        \throws     SCXUnauthorizedFileSystemAccessException    If oldPath specifies a file
        \throws     SCXFilePathNotFoundException                If no file is found at oldPath

        If newPath is a path to an existing directory, the directory
        is moved to that directory using the old name.

        This method works across disk volumes, and it does not throw an exception if the
        source and destination are the same. Note that if you attempt to replace a file by
        moving a file of the same name into that directory, you get an IOException. You
        cannot use the Move method to overwrite an existing file.

        The oldPath and newPath arguments are permitted to specify relative or
        absolute path information. Relative path information is interpreted as relative
        to the current working directory.
     */
    void SCXDirectory::Move(const SCXFilePath& oldPath, const SCXFilePath &newPath) {
        SCXFileSystem::Attributes attribs(SCXFileSystem::GetAttributes(oldPath));
        if (attribs.count(SCXFileSystem::eDirectory) == 0) {
            throw SCXUnauthorizedFileSystemAccessException(oldPath, SCXFileSystem::GetAttributes(oldPath), SCXSRCLOCATION);
        }
        SCXFileSystem::Move(oldPath, newPath);
    }

    /*--------------------------------------------------------------*/
    /**
        Determines whether the specified directory exists

        \param[in]      path        Path to directory
        \returns        true        iff the path refers to a directory (not a file) and the directory exists

        The Exists method should not be used for path validation, this method merely checks
        if the file specified in path exists. Passing an invalid path to Existsl returns false.

        Be aware that another process can potentially do something with the directory in between the
        time you call the Exists method and perform another operation on the file, such as Delete.
        A recommended programming practice is to wrap the Exists method, and the operations you
        take on the file, in a try...catch block. The Exists method can only help to ensure that the
        file will be available, it cannot guarantee it.

        The path parameter is permitted to specify relative or absolute path information.
        Relative path information is interpreted as relative to the current working directory.
        To obtain the current working directory, see GetCurrentDirectory.

        \note If path describes a file, this method returns false.
    */
    bool SCXDirectory::Exists(const SCXFilePath& path) {
        try {
            return (SCXFileSystem::GetAttributes(path).count(SCXFileSystem::eDirectory) == 1);
        } catch (SCXFilePathNotFoundException&) {
            return false;
        }
    }
        /*--------------------------------------------------------------*/
    /**
        Creates all directories and subdirectories specified by path.

        \param[in]      path        Path to directory to create.
        \returns        A DirectoryInfo as specified by path.
        \throws         SCXUnauthorizedFileSystemAccessException
        \throws         SCXFilePathNotFoundExcepiton
        \throws         SCXResourceExhaustedException
        \throws         SCXInternalErrorException

        Any and all directories specified in path are created,
        unless they already exist or unless some part of path is
        invalid. The path parameter specifies a directory path, not
        a file path. If the directory already exists, this method
        does nothing.
    */
    SCXDirectoryInfo SCXDirectory::CreateDirectory(const SCXFilePath& path) {
        if (Exists(path)) {
            return SCXDirectoryInfo(path);
        }
        SCXFilePath parentDir = path;
        parentDir.AppendDirectory(L"../");
        CreateDirectory(SCXFileSystem::CreateFullPath(parentDir));

#if defined(WIN32)
        int failure = _wmkdir(path.Get().c_str());
#elif defined(SCX_UNIX)
        std::string localizedName = SCXFileSystem::EncodePath(path);
        int failure = mkdir(localizedName.c_str(), 0777); // 0777 will result in 0777 & ~umask.
#else
#error
#endif
        if (failure) {
            switch (errno) {
            case EACCES:
            case EPERM:
            case EROFS:
            case EEXIST:
            case ENOTDIR:
                throw SCXUnauthorizedFileSystemAccessException(path,
                                                               SCXFileSystem::Attributes(),
                                                               SCXSRCLOCATION);

#if !defined(WIN32)
            case ELOOP:
#endif
            case ENAMETOOLONG:
            case ENOENT:
                throw SCXFilePathNotFoundException(path, SCXSRCLOCATION);
            case ENOMEM:
                throw SCXResourceExhaustedException(L"memory", L"Could not create directory", SCXSRCLOCATION);
            case ENOSPC:
                throw SCXResourceExhaustedException(L"disk space", L"Could not create directory", SCXSRCLOCATION);
            default:
                throw SCXInternalErrorException(L"Unexpected errno " + StrFrom(errno), SCXSRCLOCATION);
            }
        }
        return SCXDirectoryInfo(path);
    }

#if !defined(DISABLE_WIN_UNSUPPORTED)
    /*--------------------------------------------------------------*/
    /**
        Creates a temp directory.

        \returns    SCXDirectoryInfo representing newly created directory.

     */
    SCXDirectoryInfo SCXDirectory::CreateTempDirectory() {
        /**
         * The code below behaves as it does because of limitations in the
         * various temp file functions.
         *
         * \li Use the tempnam function to retrieve an "appropriate" directory
         *     to create temp directory in.
         * \li Only use the directory part of what is returned and append a
         *     pattern to use for creating temporary directory.
         * \li Use the pattern as argument to mkdtemp which is the recommended
         *     function since it avoids race conditions.
         *
         * On systems other than linux, the mkdtemp method does not exist. because of
         * this, CreateDirectory is used to create the directory with the name retrieved
         * from tempnam.
         */
        SCXFilePath pattern;
        char scxTemplate[] = "scxXXXXXX";

        int fd = mkstemp(scxTemplate);

        if (fd == -1) {
            throw SCXInternalErrorException(
                UnexpectedErrno(L"Failed to find an appropriate temporary file directory", errno),
                SCXSRCLOCATION);
        }

        close(fd);

        // Remove the file so we can use the name for a directory.
        remove(scxTemplate);

        try
        {
            pattern = SCXFileSystem::DecodePath(scxTemplate);
        }
        catch ( SCXCoreLib::SCXException& e )
        {
            SCXRETHROW(e, L"Unable to decode file path." );
        }

        // Only linux has mkdtemp.
#if defined(linux)
        pattern.SetFilename(L"scxXXXXXX");
        std::string patternString = SCXFileSystem::EncodePath(pattern);
        std::vector<char> buf;

        buf.resize( patternString.length()+1, 0 );
        strcpy(&buf[0], patternString.c_str());

        if (mkdtemp(&buf[0]) == NULL) {
            switch (errno) {
            case EACCES:
            case EPERM:
            case EROFS:
            case EEXIST:
            case ENOTDIR:
                throw SCXUnauthorizedFileSystemAccessException(pattern,
                                                               SCXFileSystem::Attributes(),
                                                               SCXSRCLOCATION);
                
            case ELOOP:
            case ENAMETOOLONG:
            case ENOENT:
                throw SCXFilePathNotFoundException(pattern, SCXSRCLOCATION);
            case ENOMEM:
                throw SCXResourceExhaustedException(L"memory", L"Could not create directory", SCXSRCLOCATION);
            case ENOSPC:
                throw SCXResourceExhaustedException(L"disk space", L"Could not create directory", SCXSRCLOCATION);
            default:
                throw SCXInternalErrorException(L"Unexpected errno " + StrFrom(errno), SCXSRCLOCATION);
            }

        }
        buf.push_back('/');
        SCXFilePath filepath = SCXFileSystem::DecodePath(&buf[0]);

        return SCXDirectoryInfo(filepath);
#else
        std::wstring newdir = pattern.Get();
        newdir.push_back(L'/');
        pattern.Set(newdir);
        mode_t oldUmask = umask(077);
        SCXDirectoryInfo retVal = CreateDirectory(pattern);
        umask(oldUmask);
        return retVal;
#endif
    }
#endif
    /*--------------------------------------------------------------*/
    /**
        Finds the paths of file system objects in a specified directory.

        \param[in] path Path to directory.
        \param[in] options Search options that can be used to filter the result. Default: eDirSearchOptionAll
        \returns A vector of SCXFilePaths.
        \throws     SCXFilePathNotFoundException if path is not found.
    */
    std::vector<SCXCoreLib::SCXFilePath> SCXDirectory::GetFileSystemEntries(
        const SCXFilePath &path, 
        const SCXDirectorySearchOptions options /*= SCXDirectorySearchOption::eDirSearchOptionAll*/)
    {
#if defined(WIN32) /* Windows is very picky about the template types - and hpux picky in the other direction */
        const 
#endif
        SCXCoreLib::SCXHandle<SCXDirectoryEnumeratorBehaviourPath> paths( new SCXDirectoryEnumeratorBehaviourPath() );
        SCXDirectoryEnumerator enumerator(paths);
        enumerator.FindFiles(path, options);
        return paths->GetFiles();
    }

    /*--------------------------------------------------------------*/
    /**
        Finds the paths of all regular files in specified directory.

        \param[in] path Path to directory.
        \returns A vector of SCXFilePaths.
        \throws     SCXFilePathNotFoundException if path is not found.
    */
    std::vector<SCXCoreLib::SCXFilePath> SCXDirectory::GetFiles(const SCXFilePath &path)
    {
        return SCXDirectory::GetFileSystemEntries(path, SCXCoreLib::eDirSearchOptionFile);
    }

    /*--------------------------------------------------------------*/
    /**
        Finds the paths of all directories in specified directory.

        \param[in] path Path to directory.
        \returns A vector of SCXFilePaths.
        \throws     SCXFilePathNotFoundException if path is not found.
    */
    std::vector<SCXCoreLib::SCXFilePath> SCXDirectory::GetDirectories(const SCXFilePath &path)
    {
        return SCXDirectory::GetFileSystemEntries(path, SCXCoreLib::eDirSearchOptionDir);
    }

    /*--------------------------------------------------------------*/
    /**
        Finds the paths of all system files in specified directory.

        \param[in] path Path to directory.
        \returns A vector of SCXFilePaths.
        \throws     SCXFilePathNotFoundException if path is not found.
    */
    std::vector<SCXCoreLib::SCXFilePath> SCXDirectory::GetSysFiles(const SCXFilePath &path)
    {
        return SCXDirectory::GetFileSystemEntries(path, SCXCoreLib::eDirSearchOptionSys);
    }
} /* namespace SCXCoreLib */


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
