/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file
 
    \brief     Implements the filename PAL
 
    \date      07-05-14 12:34:56
 

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxexception.h>

namespace SCXCoreLib
{
    /** Platform specific folder separator. */
    /*static*/ const wchar_t SCXFilePath::s_folderSeparator = 
#ifdef WIN32
        L'\\'
#else
        L'/'
#endif
        ;

    /** Platform specific suffix separator. */
    /*static*/ const wchar_t SCXFilePath::s_suffixSeparator = L'.';

    /** All supported folder separators. */
    /*static*/ const wchar_t* SCXFilePath::s_folderSeparatorsAllowed = L"/\\";

    /** All supported suffix separators. */
    /*static*/ const wchar_t* SCXFilePath::s_suffixSeparatorsAllowed = L".";

    /*----------------------------------------------------------------------------*/
    /**
       Default constructor.

       \date       2007-05-14 12:34:56

       The default constructor initializes to an empty filename/path.

    */
    SCXFilePath::SCXFilePath(void)
    {
        m_directory = std::wstring(L"");
        m_filename = std::wstring(L"");
    }


    /*----------------------------------------------------------------------------*/
    /**
       Copy constructor.
       
       \param[in] src  Another SCXFilePath instance which should be copied.
       
       \date       2007-05-14 12:34:56
       
       Will create a copy of the given SCXFilePath.

    */
    SCXFilePath::SCXFilePath(const SCXFilePath& src)
    {
        m_directory = src.m_directory;
        m_filename = src.m_filename;
    }


    /*----------------------------------------------------------------------------*/
    /**
       String constructor.

       \param[in] src  A std::wstring that should be used to initialize the filename.

       \date       2007-05-14 12:34:56

       This constructor is supplied for convenience. Using this constructor 
       results in tha same behaviour as if the default constructor was used
       followed by a call to the Set method using argument src.
       
    */
    SCXFilePath::SCXFilePath(const std::wstring& src)
    {
        Set(src);
    }

    /*----------------------------------------------------------------------------*/
    /**
       String constructor.

       \param[in] src  A wchar_t array (string) that should be used to initialize the filename.

       \date       2007-05-14 12:34:56

       This constructor is supplied for convenience. Much similar to the 
       string constructor.

    */
    SCXFilePath::SCXFilePath(const wchar_t* src)
    {
        Set(std::wstring(src));
    }

   
    /*----------------------------------------------------------------------------*/
    /**
       Dump object as string (for logging).

       \returns   The object represented as a string suitable for logging.

       \date       2007-05-14 12:34:56
     
    */ 
    const std::wstring SCXFilePath::DumpString() const
    {
        return SCXDumpStringBuilder("SCXFilePath")
            .Text("directory", m_directory)
            .Text("filename", m_filename);
    }
 
    /*----------------------------------------------------------------------------*/
    /**
       Sets the filepath.

       \param[in]  path  The complete path that should be set.
       
       \date       2007-05-14 12:34:56
       
       Method will split path in folder and filename parts as well as
       change the folder and suffix separators to the appropiate values.

    */
    void SCXFilePath::Set(const std::wstring& path)
    {
        std::wstring::size_type idx = path.find_last_of(s_folderSeparatorsAllowed);
        if (std::wstring::npos == idx)
        {
            m_directory = L"";
            m_filename = path;
        }
        else
        {
            m_directory = path.substr(0,idx+1);
            m_filename = path.substr(idx+1);
        }
        ReplaceSeparators(eSeparatorReplaceAll);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Change only the filename of the path.

       \param[in] filename  The new filename.

       \throws    SCXInvalidArgumentException if argument contains folder separator(s).

       \date      2007-05-14 12:34:56

       This method will only change the filename part of the path.

    */
    void SCXFilePath::SetFilename(const std::wstring& filename)
    {
        if (std::wstring::npos != filename.find_first_of(s_folderSeparatorsAllowed))
        {
            SCXASSERT( ! "Argument (filename) contains folder separators" );
            throw SCXInvalidArgumentException(L"filename", L"contains folder separators", SCXSRCLOCATION);
        }
        m_filename = filename;
        ReplaceSeparators(eSeparatorReplaceSuffix);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Change only the filename suffix of the path.

       \param[in] suffix  The new filename suffix.
       
       \date       2007-05-14 12:34:56

       This method will only change the filename suffix part of the path.
       This method will not add the suffix if the filename is empty.
    */
    void SCXFilePath::SetFilesuffix(const std::wstring& suffix)
    {
        std::wstring::size_type idx = m_filename.find_last_of(s_suffixSeparator);
        if (idx != std::wstring::npos)
        {
            m_filename = m_filename.substr(0, idx);
        }
        if (suffix.length() > 0 && m_filename.length() > 0)
        {
            if (0 != suffix.find_first_of(s_suffixSeparatorsAllowed))
            {
                m_filename += s_suffixSeparator;
            }
            m_filename += suffix;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Change only the directory of the path.

       \param[in] directory  The new directory.

       \date       2007-05-14 12:34:56

       This method will only change the directory part of the path.
    */
    void SCXFilePath::SetDirectory(const std::wstring& directory)
    {
        m_directory = directory;
        if (m_directory.length() > 0)
        {
            if (m_directory.length()-1 != m_directory.find_last_of(s_folderSeparatorsAllowed))
            {
                m_directory += s_folderSeparator;
            }
        }
        ReplaceSeparators(eSeparatorReplaceFolder);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve the complete file path.

       \returns    The complete path stored in the object.

       \date       2007-05-14 12:34:56

       This method will return the complete path. 

    */
    const std::wstring SCXFilePath::Get(void) const
    {
        std::wstring result = m_directory;
        result += m_filename;
        return result;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Retrieve the filename of the path.

       \returns    The filename part of the path.

       \date    2007-05-14 12:34:56

       Returns the filename of the object. If there is no filename, an empty 
       string is returned.

    */
    const std::wstring& SCXFilePath::GetFilename(void) const
    {
        return m_filename;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Retrieve the filename suffix of the path.

       \returns    The filename suffix part of the path.

       \date       2007-05-14 12:34:56

       Returns the filename suffix of the object. If there is no filename
       or suffix an empty string is returned.

    */
    const std::wstring SCXFilePath::GetFilesuffix(void) const
    {
        std::wstring::size_type idx = m_filename.find_last_of(s_suffixSeparator);
        if (idx != std::wstring::npos)
        {
            return m_filename.substr(idx+1);
        }
        return L"";
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve the directory of the path.

       \returns    The directory part of the path.

       \date       2007-05-14 12:34:56

       Returns the directory of the object. If there is only a filename
       an empty string is returned.

    */
    const std::wstring& SCXFilePath::GetDirectory(void) const
    {
        return m_directory;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Append directiories/filename to the path.
       
       \param[in] str  Data to append.
       
       \throws     SCXInvalidArgumentException if string to append contains folder
                   separator(s) and there is already a file name in the path. 

       \date       2007-05-14 12:34:56

       If the current path has no filename, both directories and/or filenames
       may be appended. If there already is a filename, only the filename may
       have data appended. If the filename is non-empty, folder separators are 
       not allowed in the argument. If the argument ends with a folder separator
       it is assumed to be a folder only append.

    */
    void SCXFilePath::Append(const std::wstring& str)
    {
        if (m_filename.length() > 0)
        { /* There is a file name already */
            if (std::wstring::npos == str.find_first_of(s_folderSeparatorsAllowed))
            { /* The string to append is only a file name */
                m_filename += str;
                ReplaceSeparators(eSeparatorReplaceSuffix);
            }
            else
            { /* The string to append contains folder separators! */
                SCXASSERT( ! "Unable to append string with folder separators since filename is set" );
                throw SCXInvalidArgumentException(L"str", L"Unable to append string with folder separators since filename is set", SCXSRCLOCATION);
            }
        }
        else
        { /* There is no filename */
            if ( ! m_directory.empty())
            {
                std::wstring what(s_folderSeparatorsAllowed);
                Set(m_directory + SCXCoreLib::StrStripL(str, what));
            }
            else
            {
                Set(str);
            }
        }
    }


    /*----------------------------------------------------------------------------*/
    /**
       Append directiories to the path.

       \param[in] directory  Directory to append.

       \date       2007-05-14 12:34:56

       Will append the given directory to the current directory keeping the 
       filename intact. Any starting folder separators will be removed unless
       the current directory is empty. A folder separator will be added to 
       the end if missing.
       
    */
    void SCXFilePath::AppendDirectory(const std::wstring& directory)
    {
        if ( ! m_directory.empty())
        {
            std::wstring what(s_folderSeparatorsAllowed);
            m_directory += SCXCoreLib::StrStripL(directory, what);
        }
        else
        {
            m_directory += directory;
        }
        if (m_directory.length()-1 != m_directory.find_last_of(s_folderSeparatorsAllowed))
        {
            m_directory += s_folderSeparator;
        }
        ReplaceSeparators(eSeparatorReplaceFolder);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Static member that returns the folder separator.

       \returns Folder separator.

       \date       2008-07-07 09:59:08

    */
    wchar_t SCXFilePath::GetFolderSeparator()
    {
        return s_folderSeparator;
    }

    /*----------------------------------------------------------------------------*/
    /**
       std::wstring cast operator.

       \returns    Path as a single std::wstring.

       \date       2007-05-14 12:34:56

       Essentially returns the same thing as the get method.
    */
    SCXFilePath::operator std::wstring (){
        return Get();
    }


    /*----------------------------------------------------------------------------*/
    /**
       Append operator.

       \param[in]  op String to append.
       \returns    Reference to current path.
       \throws     std::invalid_argument if string to append contains folder
                   separator(s) and there is already a file name in the path.

       \date       2007-05-14 12:34:56

       An append operator for convenience.
    */
    SCXFilePath& SCXFilePath::operator+= (const std::wstring& op){
        Append(op);
        return *this;
    }


    /*----------------------------------------------------------------------------*/
    /**
       Assignment operator.

       \param[in]  op Path to assign from.
       \returns       Reference to current path.

       \date       2007-05-14 12:34:56

       Replaces the current path to that of the operand.
    */
    SCXFilePath& SCXFilePath::operator= (const SCXFilePath& op){
        if (this != &op)
        {
            m_directory = op.m_directory;
            m_filename = op.m_filename;
        }
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Assignment operator for std::wstring.

       \param[in] op Path to assign as a std::wstring.
       \returns      Reference to current path.

       \date      2007-05-14 12:34:56

       Essentially a convenient wrapper for the Set method.

    */
    SCXFilePath& SCXFilePath::operator= (const std::wstring& op){
        Set(op);
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Equality operator.

       \param[in] op1  First path operand to compare.
       \param[in] op2  Second path operand to compare.
       \returns       true if the two paths have the same content, otherwise false.

       \date      2007-05-14 12:34:56

       Case sensitive equal operator to compare two paths.

    */
    bool operator== (const SCXFilePath& op1, const SCXFilePath& op2){
        return (0 == op1.m_directory.compare(op2.m_directory) &&
                0 == op1.m_filename.compare(op2.m_filename));
    }

    /*----------------------------------------------------------------------------*/
    /**
       Not Equal operator.

       \param[in] op1  First path operand to compare.
       \param[in] op2  Second path operand to compare.
       \returns       false if the two paths have the same content, otherwise true.

       \date    2007-05-14 12:34:56

       Case sensitive not equal operator to compare two paths.

    */
    bool operator!= (const SCXFilePath& op1, const SCXFilePath& op2){
        return ! (op1 == op2);
    }


    /*----------------------------------------------------------------------------*/
    /**
       Update the internal path representation to match current platform.

       \param[in] flag  What part of the internal representation should be updated.

       \date      2007-05-14 12:34:56

       This is an internal method to make sure the internal representation
       of the path is matching the platform. This method should be called
       whenever the path is changed with non SCXFilePath strings. the flag
       argument is used to avoid checking parts of the path that have not
       been changed.
    */
    void SCXFilePath::ReplaceSeparators(SeparatorReplaceFlagType flag)
    {
        if (flag & eSeparatorReplaceFolder)
        {
            for (std::wstring::size_type idx = m_directory.find_first_of(s_folderSeparatorsAllowed);
                idx != std::wstring::npos;
                idx = m_directory.find_first_of(s_folderSeparatorsAllowed, idx+1))
            {
                if (m_directory[idx] != s_folderSeparator)
                {
                    m_directory.replace(idx, 1, 1, s_folderSeparator);
                }
            }
        }

        if (flag & eSeparatorReplaceSuffix)
        {
            std::wstring::size_type idx = m_filename.find_last_of(s_suffixSeparatorsAllowed);
            if (idx != std::wstring::npos && m_filename[idx] != s_suffixSeparator) 
            {
                m_filename.replace(idx, 1, 1, s_suffixSeparator);
            }
        }
    }

} /* namespace SCXCoreLib */


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
