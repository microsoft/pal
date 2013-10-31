/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file
 
    \brief     Defines the public interface for the filepath PAL.

    \date      2007-05-14 12:34:56
 

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXFILEPATH_H
#define SCXFILEPATH_H

#include <string>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
       SCXFilePath implements a platform-independent file/folder name handler. 
    
       \date        2007-05-14 12:34:56
    
      In order to make sure file paths are handled in a consistant 
      way, this class should be used for all path handling.
    
    */
    class SCXFilePath
    {
    public:
        SCXFilePath(void);
        SCXFilePath(const SCXFilePath&);
        SCXFilePath(const std::wstring&);
        SCXFilePath(const wchar_t*);

        const std::wstring DumpString() const;

    public:
        void Set(const std::wstring&);
        void SetFilename(const std::wstring&);
        void SetFilesuffix(const std::wstring&);
        void SetDirectory(const std::wstring&);

        const std::wstring Get(void) const;
        const std::wstring& GetFilename(void) const;
        const std::wstring GetFilesuffix(void) const;
        const std::wstring& GetDirectory(void) const;

        void Append(const std::wstring&);
        void AppendDirectory(const std::wstring&);
        
        static wchar_t GetFolderSeparator();

        operator std::wstring ();
        SCXFilePath& operator+= (const std::wstring&);
        SCXFilePath& operator= (const SCXFilePath&);
        SCXFilePath& operator= (const std::wstring&);
        friend bool operator== (const SCXFilePath&, const SCXFilePath&);
        friend bool operator!= (const SCXFilePath&, const SCXFilePath&);

    protected:
        //! Internal bitmask enum specifying action of ReplaceSeparators()
        enum SeparatorReplaceFlagType
        {
            eSeparatorReplaceFolder = 0x1,
            eSeparatorReplaceSuffix = 0x2,
            eSeparatorReplaceAll = eSeparatorReplaceFolder | eSeparatorReplaceSuffix
        };
            
        void ReplaceSeparators(SeparatorReplaceFlagType flag);

    protected:
        //! Contains the directory name at any one time
        std::wstring m_directory;
        //! Contains the file name 
        std::wstring m_filename;

        static const wchar_t s_folderSeparator;
        static const wchar_t* s_folderSeparatorsAllowed;
        static const wchar_t s_suffixSeparator;
        static const wchar_t* s_suffixSeparatorsAllowed;
    };
} /* namespace SCXCoreLib */

#endif /* SCXFILEPATH_H */
/*------------------E-N-D---O-F---F-I-L-E-------------------------------*/
