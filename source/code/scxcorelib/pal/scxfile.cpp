/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
    \file

    \brief     Platform independent file management interface

    \date      2007-08-24 12:00:00

 */
/*----------------------------------------------------------------------------*/


#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <scxcorelib/scxoserror.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxdirectoryinfo.h>

/** To use API mmap and munmap*/
#include <sys/mman.h>

#ifndef MAP_FAILED  //!< returned error value described in API mmap.
#define MAP_FAILED ((void *) -1)
#endif

namespace SCXCoreLib {
/*--------------------------------------------------------------*/

/**
    Initializes a new instance of the FileInfo class, which acts as a wrapper for a file path.

    \param[in]  path    Path to the file, full as well as relative
    \param[in]  pStat   A stat struct to use when initializing. If zero (default) a new one will be used.

    There is no need to call Refresh before using the instance,
    all information is already fresh. If a relative path is given it is
    resolved relative to the currently active working directory to form
    a fully qualified path. In that way we make sure which file we actually
    refer to.
*/
    SCXFileInfo::SCXFileInfo(const SCXFilePath& path, SCXFileSystem::SCXStatStruct* pStat /* = 0 */)
        : SCXFileSystemInfo(SCXFileSystem::CreateFullPath(path), path, pStat)
    {
    }

    /*--------------------------------------------------------------*/
    /**
       Did the referred file exist last time we checked?
       \returns whether the path referes to a file or not

     */
    bool SCXFileInfo::Exists() const {
        if (PathExists() && GetAttributes().count(SCXFileSystem::eDirectory) == 0)
        {
            return true;
        }
        return false;
    }

    /*--------------------------------------------------------------*/
    /**
           Delete the file

    */
    void SCXFileInfo::Delete() {
        SCXFile::Delete(GetFullPath());
        InitializePathExists(false);
    }

    /*--------------------------------------------------------------*/
    /**
           Reread file information

    */
    void SCXFileInfo::Refresh() {
        ReadFromDisk();
    }

    /*-----------------------------------------------------------*/
    /**
           Dump class content to string

           \returns  The dumped content

    */
    const std::wstring SCXFileInfo::DumpString() const {
        std::wstring exists = Exists() ? L"existing " : L"nonexisting ";
        return exists + SCXFileSystemInfo::DumpString()
            + L" attributes " + SCXFileSystem::AsText(GetAttributes());
    }

    /*--------------------------------------------------------------*/
    /**
        Convenience method to retrieve the directory part of the full path

        \returns    Directory part of the path

    */
    const SCXFilePath SCXFileInfo::GetDirectoryPath() const {
        return SCXFilePath(GetFullPath().GetDirectory());
    }

    /*--------------------------------------------------------------*/
    /**
        Convenience method to check if eWritable is among the file attributes

        \returns    if the file is known to be writable or not

     */
    bool SCXFileInfo::isWritable() const {
        return GetAttributes().count(SCXFileSystem::eWritable) > 0;
    }

    /*--------------------------------------------------------------*/
    /**
        Determines whether the specified file exists

        \param[in]      path        Path to file
        \returns        true        iff the path refers to a file (not a directory) and the file exists

        The Exists method should not be used for path validation, this method merely checks
        if the file specified in path exists. Passing an invalid path to Exists returns false.

        Be aware that another process can potentially do something with the file in between the
        time you call the Exists method and perform another operation on the file, such as Delete.
        A recommended programming practice is to wrap the Exists method, and the operations you
        take on the file, in a try...catch block. The Exists method can only help to ensure that the
        file will be available, it cannot guarantee it.

        The path parameter is permitted to specify relative or absolute path information.
        Relative path information is interpreted as relative to the current working directory.
        To obtain the current working directory, see GetCurrentDirectory.

        \note If path describes a directory, this method returns false.
    */
    bool SCXFile::Exists(const SCXFilePath& path) {
#if defined(WIN32)
        struct __stat64 buf;
        int failure = _wstat64(path.Get().c_str(), &buf );
#elif defined(SCX_UNIX)
        SCXFileSystem::SCXStatStruct buf;
        std::string localizedName = SCXFileSystem::EncodePath(path);
        int failure = SCXFileSystem::Stat(localizedName.c_str(), &buf );
#else
#error
#endif
        return !failure && !(buf.st_mode & S_IFDIR);
    }

    /*--------------------------------------------------------------*/
    /**
        Sets the specified FileAttributes of the file on the specified path

        \param[in]  path            The path to the file.
        \param[in]  attributes      The desired FileAttributes, such as Readable and Writable
        \throws     SCXUnauthorizedFileSystemAccessException    The caller does not have the required permission
                                                                or path is a directory
        \throws     SCXFilePathNotFoundException                If no file is found at path

        The path parameter is permitted to specify relative or absolute path information.
        Relative path information is interpreted as relative to the current working directory.
        To obtain the current working directory, see GetCurrentDirectory.

        It is not possible to change the compression status of a File object using the
        SetAttributes method

     */
    void SCXFile::SetAttributes(const SCXFilePath& path, const SCXFileSystem::Attributes& attributes) {
#if defined(WIN32)
        struct __stat64 buf;
        int failure = _wstat64(path.Get().c_str(), &buf );
#elif defined(SCX_UNIX)
        SCXFileSystem::SCXStatStruct buf;
        std::string localizedPath = SCXFileSystem::EncodePath(path);
        int failure = SCXFileSystem::Stat(localizedPath.c_str(), &buf );
#else
#error
#endif
        if (!failure) {
            if (buf.st_mode & S_IFDIR) {
                throw SCXUnauthorizedFileSystemAccessException(path, SCXFileSystem::GetAttributes(path), SCXSRCLOCATION);
            } else {
                SCXFileSystem::SetAttributes(path, attributes);
            }
        }
    }

    /*--------------------------------------------------------------*/
    /**
        Deletes the specified file. An exception is not thrown if the specified file does not exist

        \param[in]  path    The path to the file to be deleted.
        \throws     SCXUnauthorizedFileSystemAccessException    The caller does not have the required permission
                                                                or path is a directory or path specified a read-only file

        The path parameter is permitted to specify relative or absolute path information. Relative
        path information is interpreted as relative to the current working directory. To obtain
        the current working directory, see GetCurrentDirectory
     */
    void SCXFile::Delete(const SCXFilePath& path) {
#if defined(WIN32)
        int failure = _wremove(path.Get().c_str());
#elif defined(SCX_UNIX)
        std::string localizedPath = SCXFileSystem::EncodePath(path);
        int failure = remove(localizedPath.c_str());
#else
#error
#endif
        if (failure) {
            if (errno == EACCES || errno == EBUSY || errno == EPERM || errno == EROFS) {
                throw SCXUnauthorizedFileSystemAccessException(path, SCXFileSystem::GetAttributes(path), SCXSRCLOCATION);
            } else if (errno == ENOENT) {
                const int existenceOnly = 00;
#if defined(WIN32)
                failure = _waccess(path.Get().c_str(), existenceOnly);
#elif defined(SCX_UNIX)
                failure = access(localizedPath.c_str(), existenceOnly);
#else
#error
#endif
                bool fileStillExists = !failure;
                if (fileStillExists) {
                    // We got ENOENT when trying to remove the file,
                    // but appearently the file exists. That means that
                    // the file is not a file but a directory
                    throw SCXUnauthorizedFileSystemAccessException(path, SCXFileSystem::GetAttributes(path), SCXSRCLOCATION);
                } if (errno == EACCES) {
                    throw SCXUnauthorizedFileSystemAccessException(path, SCXFileSystem::GetAttributes(path), SCXSRCLOCATION);
                } else if (errno == EINVAL) {
                    throw SCXInvalidArgumentException(L"path", L"Invalid format of path " + path.Get(), SCXSRCLOCATION);
                } else if (errno != ENOENT) {
                    std::wstring problem(L"Failed to delete " + path.Get());
                    throw SCXInternalErrorException(UnexpectedErrno(problem, errno), SCXSRCLOCATION);
                }
            } else {
                std::wstring problem(L"Failed to delete " + path.Get());
                throw SCXInternalErrorException(UnexpectedErrno(problem, errno), SCXSRCLOCATION);
            }
        }
    }

    /*--------------------------------------------------------------*/
    /**
        Moves a specified file to a new location, providing the option to specify a
        new file name.

        \param[in]  oldPath     Existing file, not direcoty, to be moved
        \param[in]  newPath     New path, directory or file

        \throws     SCXUnauthorizedFileSystemAccessException    If oldPath specifies a directory
        \throws     SCXFilePathNotFoundException                If no file is found at oldPath

        If newPath is a path not to a file but to an existing directory the file
        is moved to that directory using the old name.

        This method works across disk volumes, and it does not throw an exception if the
        source and destination are the same. Note that if you attempt to replace a file by
        moving a file of the same name into that directory, you get an IOException. You
        cannot use the Move method to overwrite an existing file.

        The oldPath and newPath arguments are permitted to specify relative or
        absolute path information. Relative path information is interpreted as relative
        to the current working directory.
     */
    void SCXFile::Move(const SCXFilePath& oldPath, const SCXFilePath &newPath) {
        SCXFileSystem::Attributes attribs(SCXFileSystem::GetAttributes(oldPath));
        if (attribs.count(SCXFileSystem::eDirectory) > 0) {
            throw SCXUnauthorizedFileSystemAccessException(newPath, SCXFileSystem::GetAttributes(newPath), SCXSRCLOCATION);
        }
        SCXFileSystem::Move(oldPath, newPath);
    }

    /*--------------------------------------------------------------*/
    /**
        Opens a file stream assuming the file is encoded according to the system locale

        \param[in]  file    The file to open
        \param[in]  mode    How to open it, explicitly.

        \throws     SCXFilePathNotFoundException
        \throws     SCXUnauthorizedFileSystemAccessException
        \throws     SCXNotSupportedException
        \throws     InvalidArgumentException Arguments

        Unlike STL there is no implicit (default) mode, the requested mode has to explicitly stated.
        The content of the file is assumed to be encoded according to system default.
     */
    SCXHandle<std::wfstream> SCXFile::OpenWFstream(const SCXFilePath& file, std::ios_base::openmode mode) {
        if (!(mode & std::ios::in) && !(mode & std::ios::out)) {
            throw SCXInvalidArgumentException(L"mode", L"Specify ios::in or ios::out, or both", SCXSRCLOCATION);
        }
        if (mode & std::ios::binary) {
            throw SCXNotSupportedException(L"wide streams must not be binary", SCXSRCLOCATION);
        }
#if defined(WIN32)
        SCXHandle<std::wfstream> streamPtr(new std::wfstream(file.Get().c_str(), mode));
#elif defined(SCX_UNIX)
        SCXHandle<std::wfstream> streamPtr(new std::wfstream(SCXFileSystem::EncodePath(file).c_str(), mode));
#else
#error
#endif
        if (streamPtr->good()) {
            SCXFileSystem::Attributes attribs(SCXFileSystem::GetAttributes(file));
            if (attribs.count(SCXFileSystem::eDirectory) > 0) {
                throw SCXUnauthorizedFileSystemAccessException(file, attribs, SCXSRCLOCATION);
            }
        } else {
            SCXFileInfo info(file);
            if (mode & std::ios::in) {
                if (!info.PathExists()) {
                    throw SCXFilePathNotFoundException(file, SCXSRCLOCATION);
                } else {
                    throw SCXUnauthorizedFileSystemAccessException(file,
                            SCXFileSystem::GetAttributes(file), SCXSRCLOCATION);
                }
            } else if (mode & std::ios::out) {
                throw SCXUnauthorizedFileSystemAccessException(file,
                    SCXFileSystem::GetAttributes(file), SCXSRCLOCATION);
            } else {
                SCXASSERT(!"Invalid mode");
            }
        }
        return streamPtr;
    }

    /*--------------------------------------------------------------*/
    /**
        Opens a file stream

        \param[in]  file    The file to open
        \param[in]  mode    How to open it, explicitly.

        \throws     SCXFilePathNotFoundException
        \throws     SCXUnauthorizedFileSystemAccessException
        \throws     InvalidArgumentException Arguments

        Unlike STL there is no implicit (default) mode, the requested mode has to explicitly stated
     */
    SCXHandle<std::fstream> SCXFile::OpenFstream(const SCXFilePath& file, std::ios_base::openmode mode) {
        if (!(mode & std::ios::in) && !(mode & std::ios::out)) {
            throw SCXInvalidArgumentException(L"mode", L"Specify ios::in or ios::out, or both", SCXSRCLOCATION);
        }
#if defined(WIN32)
        SCXHandle<std::fstream> streamPtr(new std::fstream(file.Get().c_str(), mode));
#elif defined(SCX_UNIX)
        SCXHandle<std::fstream> streamPtr(new std::fstream(SCXFileSystem::EncodePath(file).c_str(), mode));
#else
#error
#endif
        if (streamPtr->good()) {
            SCXFileSystem::Attributes attribs(SCXFileSystem::GetAttributes(file));
            if (attribs.count(SCXFileSystem::eDirectory) > 0) {
                throw SCXUnauthorizedFileSystemAccessException(file, attribs, SCXSRCLOCATION);
            }

        } else {
            SCXFileInfo info(file);
            if (mode & std::ios::in) {
                if (!info.PathExists()) {
                    throw SCXFilePathNotFoundException(file, SCXSRCLOCATION);
                } else {
                    throw SCXUnauthorizedFileSystemAccessException(file,
                            (SCXFileSystem::GetAttributes(file)), SCXSRCLOCATION);
                }
            } else if (mode & std::ios::out) {
                throw SCXUnauthorizedFileSystemAccessException(file,
                    SCXFileSystem::GetAttributes(file), SCXSRCLOCATION);
            } else {
                throw SCXInvalidArgumentException(L"mode", L"Must specify ios:in or ios:out", SCXSRCLOCATION);
            }
        }
        return streamPtr;
    }

    /*--------------------------------------------------------------*/
    /**
        Reads as many lines of the file at the specified position in the file system as possible.

        \param[in]  source  Position of the file in the filesystem
        \param[in]  lines   Lines that were read
        \param[out] nlfs        Newline symbols found used in the file

        \throws     SCXUnauthorizedFileSystemAccessException
        \throws     SCXLineStreamPartialReadException       Line too long to be stored in a std::wstring

        The content of the file is assumed to be encoded according to system default.
        Handles newline symbols in a platform independent way, that is, may
        be used to read a stream originating on one platform on another platform.
        If the lines are to be written back to the originating system
        the same nlf should in general be used, if we do not have other information.

        \note Will return no lines if file not found.
     */
    void SCXFile::ReadAllLines(const SCXFilePath& source, std::vector<std::wstring>& lines, SCXStream::NLFs& nlfs) {
        try {
            SCXHandle<std::wfstream> stream(OpenWFstream(source, std::ios::in));
            SCXStream::ReadAllLines(*stream, lines, nlfs);
        } catch (SCXFilePathNotFoundException&) {
            lines.clear();
        }
    }

    /*--------------------------------------------------------------*/
    /**
        Reads as many lines of the UTF8 encoded file at the specified position in the file system as possible

        \param[in]  source  Position of the file in the filesystem
        \param[in]  lines   Lines that was read
        \param[out] nlfs        Newline symbols found used in the file

        \throws     SCXUnauthorizedFileSystemAccessException
        \throws     SCXLineStreamPartialReadException       Line to long to be stored in a std::wstring
        \throws     SCXLineStreamContentException           Invalid byte sequence according to UTF8

        \note Will return no lines if file not found.

        Handles newline symbols in a platform independent way, that is, may
        be used to read a stream originating on one platform on another platform.
        If the lines is to be written back to the originating system
        the same nlf should in general be used, if we do not have other information.

     */
    void SCXFile::ReadAllLinesAsUTF8(const SCXFilePath& source, std::vector<std::wstring>& lines, SCXStream::NLFs& nlfs) {
        try {
            SCXHandle<std::fstream> stream(OpenFstream(source, std::ios::in));
            SCXStream::ReadAllLinesAsUTF8(*stream, lines, nlfs);
        } catch (SCXFilePathNotFoundException&) {
            lines.clear();
        }
    }
    /*--------------------------------------------------------------*/
    /**
       Opens a file and reads all available bytes.

       \param path Path to file to read.
       \param buf Buffer to read data to.
       \param size Size of the given buffer.
       \param offset Offset in file where to start read. Default: 0 (start of file)

       \returns Number of bytes read and stored in given buffer.

       \throws SCXErrnoException if any system call fails with an unexpected error.
    */
#if defined(DISABLE_WIN_UNSUPPORTED)
    size_t SCXFile::ReadAvailableBytes(const SCXFilePath& , char* , size_t , size_t /*= 0*/) {
        throw SCXNotSupportedException(L"non-blocking reads not supported on windows", SCXSRCLOCATION);
#else
    size_t SCXFile::ReadAvailableBytes(const SCXFilePath& path, char* buf, size_t size, size_t offset /*= 0*/) {
        int fd = open(SCXFileSystem::EncodePath(path).c_str(), 0, O_RDONLY);
        if (-1 == fd) {
            throw SCXErrnoException(L"open(" + path.Get() + L")", errno, SCXSRCLOCATION);
        }
        int fd_flags = fcntl(fd, F_GETFL);
        if (fd_flags < 0) {
            close(fd);
            throw SCXErrnoException(L"fcntl(F_GETFL, " + path.Get() + L")", errno, SCXSRCLOCATION);
        }
        if (-1 == fcntl(fd, F_SETFL, fd_flags | O_NONBLOCK)) {
            close(fd);
            throw SCXErrnoException(L"fcntl(F_SETFL, O_NONBLOCK, " + path.Get() + L")", errno, SCXSRCLOCATION);
        }
        if (-1 == lseek(fd, offset, SEEK_SET)) {
            close(fd);
            if (ESPIPE == errno) {
                return 0;
            }
            throw SCXErrnoException(L"lseek(" + path.Get() + L")", errno, SCXSRCLOCATION);
        }
        size_t r = 0;
        while (r < size) { // Using a loop to read required on solaris.
            size_t t = read(fd, buf+r, size-r);
            if (static_cast<size_t>(-1) == t) {
                if (EAGAIN == errno) {
                    break;
                }
                close(fd);
                throw SCXErrnoException(L"read(" + path.Get() + L")", errno, SCXSRCLOCATION);
            }
            r += t;
            if (0 == t) {
                break;
            }
        }
        close(fd);
        return r;
#endif
    }

    /*--------------------------------------------------------------*/
    /**
      Opens a device system file and reads specified available bytes.

      \param path Path to device system file to read.
      \param buf Buffer to read data to as unsigned char.
      \param size Size of the given buffer.
      \param offset Offset in file where to start read. Default: 0 (start of file)
      \returns 0 if read was successful, otherwise UNIX errno code of the error that caused failure.

      \date 11-01-26 15:45:00
      \author kesheng tao
     */
#if defined(DISABLE_WIN_UNSUPPORTED)
    int SCXFile::ReadAvailableBytesAsUnsigned(const SCXFilePath& , unsigned char* , size_t , size_t /*= 0*/) {
            throw SCXNotSupportedException(L"Reads not supported on windows", SCXSRCLOCATION);
#else
    int SCXFile::ReadAvailableBytesAsUnsigned(const SCXFilePath& path, unsigned char* buf, size_t size, size_t offset /*= 0*/) {
        int fd = open(SCXFileSystem::EncodePath(path).c_str(),O_RDONLY);
        if (-1 == fd) {
            return errno;
        }

        size_t remainder;
        void *pDevice = NULL;
#ifdef _SC_PAGESIZE  //!< Size of a page in bytes
        remainder = offset% sysconf(_SC_PAGESIZE);
#else
        remainder = offset% getpagesize();
#endif
        //
        //For API mmap, the sixth parameter mmapoffset must be a multiple of pagesize.
        //
        size_t mmapoffset = offset - remainder;
        pDevice = mmap(NULL, remainder + size, PROT_READ, MAP_SHARED, fd, mmapoffset);
        if (MAP_FAILED == pDevice) {
            int ret = errno;
            close(fd);
            return ret;
        }

        memcpy(buf, reinterpret_cast<unsigned char*>(pDevice) + remainder, size);

        munmap(reinterpret_cast<char*>(pDevice), remainder + size);
        close(fd);

        return 0;
#endif
    }

    /*--------------------------------------------------------------*/
    /**
       Seek get position to absolute position

       \param[in]  source stream
       \param[in]  pos    Absolute position to seek to.
    */
    void SCXFile::SeekG(std::wfstream& source, std::wstreampos pos) {
#if defined(linux) && defined(PF_DISTRO_SUSE) && (PF_MAJOR==9)
        // seekg does not seem to work on suse9
        // It moves the position pos*4 instead of just pos.
        std::wstreampos current = source.tellg();
        if (current > pos) {
            source.seekg(0);
            source.ignore(pos);
        } else if (current < pos) {
            source.ignore(pos-current);
        }
#else
        source.seekg(pos);
#endif
    }


#if !defined(DISABLE_WIN_UNSUPPORTED)
    /*--------------------------------------------------------------*/
    /**
        Creates a temp file and writes to it

        \param[in]  fileContent    Content to write to file.
        \returns    Complete path of newly created file.

     */
    SCXFilePath SCXFile::CreateTempFile(const std::wstring& fileContent, const std::wstring& tmpDir /* = "/tmp/" */) {
        /**
         * The code below behaves as it does because of limitations in the
         * various temp file functions.
         *
         * \li Use the tempnam function to retrieve an "appropriate" directory
         *     to store files in.
         * \li Only use the directory part of what is returned and append a
         *     pattern to use for creating temporary file.
         * \li Use the pattern as argument to mkstemp which is the recommended
         *     function since it avoids race conditions.
         *
         */
        SCXFilePath pattern;
#if defined(WIN32)
        char* fp = tempnam(0, 0);
        if (fp == 0) {
            throw SCXInternalErrorException(
                UnexpectedErrno(L"Failed to find an appropriate temporary file directory", errno),
                SCXSRCLOCATION);
        }

        try
        {
            pattern = SCXFileSystem::DecodePath(fp);
        }
        catch ( SCXCoreLib::SCXException& e )
        {
            free(fp);
            fp = 0;
            SCXRETHROW(e, L"Unable to decode file path." );
        }
        free(fp);
        fp = 0;
#else
        pattern = SCXFileSystem::DecodePath(StrToUTF8(tmpDir));
        if(!SCXCoreLib::SCXDirectory::Exists(pattern))
        {
        	throw SCXCoreLib::SCXFilePathNotFoundException(pattern.GetDirectory(), SCXSRCLOCATION);
        }
#endif

        pattern.SetFilename(L"scxXXXXXX");
        std::string patternString = SCXFileSystem::EncodePath(pattern);
        std::vector<char> buf;

        buf.resize( patternString.length()+1, 0 );
        strcpy(&buf[0], patternString.c_str());

        mode_t oldUmask = umask(077);
        int fileDescriptor = mkstemp(&buf[0]);
        umask(oldUmask);

        if (fileDescriptor == -1) {
            std::wstring problem(L"Failed to create temporary file from pattern " + pattern.Get());
            throw SCXInternalErrorException(UnexpectedErrno(problem, errno), SCXSRCLOCATION);
        }

        SCXFilePath filepath = SCXFileSystem::DecodePath(&buf[0]);

        std::ostringstream fileContentStream;
        SCXStream::WriteAsUTF8(fileContentStream, fileContent);
        std::string fileContentUTF8 = fileContentStream.str();
        ssize_t written = write(fileDescriptor, fileContentUTF8.c_str(), fileContentUTF8.length());
        if (written == -1) {
            std::wstring problem(L"Failed to write to temporary file " + filepath.Get());
            close(fileDescriptor);
            throw SCXInternalErrorException(UnexpectedErrno(problem, errno), SCXSRCLOCATION);
        }

        close(fileDescriptor);
        return filepath;
    }
#endif

    /*--------------------------------------------------------------*/
    /**
     * Retrieves preferred kind of new line of a particular path. May
     * take into account the kind of file system the path refers to
     * as well as new lines currently used if the path refers to an
     * already existing file.
     * \returns Preferred kind of new line
     * \note Right now the implementation is simplified, it just returns
     * SCXFileSystem::GetDefaultNewLine()
     */
    SCXStream::NLF SCXFile::GetPreferredNewLine(const SCXFilePath&) {
        return SCXFileSystem::GetDefaultNewLine();
    }

    /*--------------------------------------------------------------*/
    /**
     * Writes all lines to a file using preferred kind of new line
     * \param[in]   target      Stream to be written to
     * \param[in]   lines       Lines to be written
     * \param[in]   mode        How to open the file
     * No new line is appended after all lines.
     * If mode is "append" an extra new line is prepended before the lines.
     */
    void SCXFile::WriteAllLines(const SCXFilePath& target, const std::vector<std::wstring>& lines, std::ios_base::openmode mode) {
        WriteAllLines(target, lines, mode, GetPreferredNewLine(target));
    }

    /*--------------------------------------------------------------*/
    /**
     * Writes all lines to a file using specified kind of new line
     * \param[in]   target      Stream to be written to
     * \param[in]   lines       Lines to be written
     * \param[in]   mode        How to open the file
     * \param[in]   nlf         Kind of new line to write
     * No new line is appended after all lines.
     * If mode is "append" an extra new line is prepended before the lines.
     */
    void SCXFile::WriteAllLines(const SCXFilePath& target, const std::vector<std::wstring>& lines, std::ios_base::openmode mode,
            SCXStream::NLF nlf) {
        SCXHandle<std::wfstream> stream(OpenWFstream(target, mode));
        if (lines.size() > 0) {
            if (mode & std::ios_base::app) {
                SCXStream::WriteNewLine(*stream, nlf);
            }
            SCXStream::Write(*stream, lines.at(0));
            for (std::wstring::size_type lineNr = 1; lineNr < lines.size(); lineNr++) {
                SCXStream::WriteNewLine(*stream, nlf);
                SCXStream::Write(*stream, lines.at(lineNr));
            }
        }
    }

    /*--------------------------------------------------------------*/
    /**
     * Writes all lines to a UTF8 encoded file using preferred kind of new line
     * \param[in]   target      Stream to be written to
     * \param[in]   lines       Lines to be written
     * \param[in]   mode        How to open the file
     * No new line is appended after all lines.
     * If mode is "append" an extra new line is prepended before the lines.
     */
    void SCXFile::WriteAllLinesAsUTF8(const SCXFilePath& target,
                const std::vector<std::wstring>& lines, std::ios_base::openmode mode) {
        WriteAllLinesAsUTF8(target, lines, mode, GetPreferredNewLine(target));
    }

    /*--------------------------------------------------------------*/
    /**
     * Writes all lines to a UTF8 encoded file using specified kind of newline
     * \param[in]   target      Stream to be written to
     * \param[in]   lines       Lines to be written
     * \param[in]   mode        How to open the file
     * \param[in]   nlf         Kind of newline to write
     * No newline is appended after all lines.
     * If mode is "append" an extra newline is prepended before the lines.
     */
    void SCXFile::WriteAllLinesAsUTF8(const SCXFilePath& target, const std::vector<std::wstring>& lines, std::ios_base::openmode mode,
            SCXStream::NLF nlf) {
        SCXHandle<std::fstream> stream(OpenFstream(target, mode));
        if (lines.size() > 0) {
            if (mode & std::ios_base::app) {
                SCXStream::WriteNewLineAsUTF8(*stream, nlf);
            }
            SCXStream::WriteAsUTF8(*stream, lines.at(0));
            for (std::wstring::size_type lineNr = 1; lineNr < lines.size(); lineNr++) {
                SCXStream::WriteNewLineAsUTF8(*stream, nlf);
                SCXStream::WriteAsUTF8(*stream, lines.at(lineNr));
            }
        }
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
