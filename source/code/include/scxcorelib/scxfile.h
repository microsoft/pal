/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
    \file

    \brief     Platform independent file management interface

    \date      2007-08-24 12:00:00

 */
/*----------------------------------------------------------------------------*/
#ifndef SCXFILE_H 
#define SCXFILE_H
 
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxstream.h>
#include <string>
#include <fstream>
#include <vector>
#include <scxcorelib/scxfilesystem.h>

namespace SCXCoreLib {

    /*----------------------------------------------------------------------------*/
    /**
        Provides instance methods for the creation, copying, deletion, moving, and opening 
        of files, and aids in the creation of FileStream objects. This class cannot be inherited. 
        
        
        Use the FileInfo class for typical operations such as copying, moving, renaming, creating, 
        opening, deleting, and appending to files.
        
        Many of the FileInfo methods return other I/O types when you create or open files. 
        You can use these other types to further manipulate a file. For more information, 
        see specific FileInfo members.
        
        If you are going to reuse an object several times, consider using the instance method of 
        FileInfo instead of the corresponding static methods of the File class.
        
        By default, full read/write access to new files is granted to all users.
     */
    class SCXFileInfo : public SCXFileSystemInfo {
    public:    
        explicit SCXFileInfo(const SCXFilePath& path, SCXCoreLib::SCXFileSystem::SCXStatStruct* pStat = 0); 

        bool Exists() const;
        const SCXFilePath GetDirectoryPath() const;
        bool isWritable() const;
 
        void Refresh(); 
        void Delete();
        
        const std::wstring DumpString() const;
    };

    /*----------------------------------------------------------------------------*/
    /**
        Provides static methods for the creation, copying, deletion, moving, 
        and opening of files, and aids in the creation of stream objects. 
    
        Use the File class for typical operations such as copying, moving, renaming, creating, 
        opening, deleting, and appending to files. You can also use the File class to get and 
        set file attributes or date/time information related to the creation, access, and writing 
        of a file.
    
        Many of the File methods return other I/O types when you create or open files. You can use
        these other types to further manipulate a file. For more information, see specific 
        File members.
    
        Because all File methods are static, it might be more efficient to use a File method rather 
        than a corresponding FileInfo instance method if you want to perform only one action. All File
        methods require the path to the file that you are manipulating.
    
        If you are going to reuse an object several times, consider using the corresponding instance 
        method of FileInfo instead, because the security check will not always be necessary.
    
        By default, full read/write access to new files is granted to all users
    
    */
    class SCXFile {
    public:
        static bool Exists(const SCXFilePath&);
        static void SetAttributes(const SCXFilePath&, const SCXFileSystem::Attributes& attributes);
        static void Delete(const SCXFilePath&);
        static void Move(const SCXFilePath& oldPath, const SCXFilePath &newPath);
        static SCXHandle<std::wfstream> OpenWFstream(const SCXFilePath& file, std::ios_base::openmode mode);
        static SCXHandle<std::fstream> OpenFstream(const SCXFilePath& file, std::ios_base::openmode mode);
        static void ReadAllLines(const SCXFilePath& source, std::vector<std::wstring>& lines, SCXStream::NLFs&);
        static void ReadAllLinesAsUTF8(const SCXFilePath& source, std::vector<std::wstring>& lines, SCXStream::NLFs&);
        static size_t ReadAvailableBytes(const SCXFilePath& path, char* buf, size_t size, size_t offset = 0);
        static int ReadAvailableBytesAsUnsigned(const SCXFilePath& path, unsigned char* buf, size_t size, size_t offset = 0);
        static void SeekG(std::wfstream& source, std::wstreampos pos);

#if !defined(DISABLE_WIN_UNSUPPORTED)        
        static SCXFilePath CreateTempFile(const std::wstring& fileContent, const std::wstring& tmpDir=L"/tmp/");
#endif

        static SCXStream::NLF GetPreferredNewLine(const SCXFilePath&);

        static void WriteAllLines(const SCXFilePath& target, const std::vector<std::wstring>& lines, std::ios_base::openmode mode);
        static void WriteAllLines(const SCXFilePath& target, const std::vector<std::wstring>& lines, std::ios_base::openmode mode, 
                                    SCXStream::NLF nlf);
        static void WriteAllLinesAsUTF8(const SCXFilePath& target, const std::vector<std::wstring>& lines, std::ios_base::openmode mode);
        static void WriteAllLinesAsUTF8(const SCXFilePath& target, const std::vector<std::wstring>& lines, std::ios_base::openmode mode, 
                                        SCXStream::NLF);
    private:
        /// Prevents the class from beeing instantiated as well as inherited from
        SCXFile();
    };

    
    /*----------------------------------------------------------------------------*/
    /**
        This class is a RAII support class for normal FILE*
    
        \date        2008-07-02
    */
    class SCXFileHandle
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] file  File to handle.
        */
        SCXFileHandle(FILE* file) : m_File(file) {};
        /*----------------------------------------------------------------------------*/
        /**
            Destructor - closes file
        */
        ~SCXFileHandle() { CloseFile(); };
        /*----------------------------------------------------------------------------*/
        /**
            Access method for the file

            \returns the stored file pointer
        */
        FILE* GetFile() { return m_File; };
        /*----------------------------------------------------------------------------*/
        /**
            Close the file
        */
        void CloseFile() { if (m_File) fclose(m_File); m_File = NULL; };
    private:
        FILE* m_File; //!< The file
    };

}

#endif  /* SCXFILE_H */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
