/*------------------------------------------------------------------------------
    Copyright (C) 2007, Microsoft Corp.

 */
/**
    \file

    \brief     Platform independent file globbing interface


 */
/*----------------------------------------------------------------------------*/
#ifndef SCXGLOB_H
#define SCXGLOB_H

#include <glob.h>
#include <string>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxfilepath.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
     *  SCXGlob implements a platform-independent filename/directory globbing utility.
     *
     *  SCXGlob treats the following search patterns to be invalid and throws 
	 *  SCXCoreLib::SCXInvalidArgumentException when encountered:
     *  \li an empty string; and
     *  \li relative directory path such as ./ or ../
     *
     *  SCXGlob always excludes /. or /.. from a set of matching file path names.
     */
    class SCXGlob
    {
    public:
       /*----------------------------------------------------------------------------*/
       /**
        *  Ctor for SCXGlob that takes a SCXFilePath search pattern string
        *
        *  \param[in] pttrn: the search pattern string.
		*  
        */
        explicit SCXGlob(const SCXFilePath &pttrn);

       /*----------------------------------------------------------------------------*/
       /**
        *  Ctor for SCXGlob that takes a wide char search pattern string.
        *
        *  \param[in] pttrn: the search pattern string.
		*  
		*  \note This form of constructor is kept for cases you really need fine-
		*        level control of pattern, where for some reason SCXFilePath 
		*        limits the expression. Avoid it if you can. 
        */
        explicit SCXGlob(const std::wstring &pttrn);

       /*----------------------------------------------------------------------------*/
       /**
        *  Dtor for SCXGlob.
        */
        ~SCXGlob();

       /*----------------------------------------------------------------------------*/
       /**
        *  Accessor for retrieving the value of the back-slash escaping flag.
        *  Default value for the back-slash escaping flag is ON.
        *
        *  \returns true if the back-slash escaping is on; false otherwise.
        */
        bool BackSlashEscapeState() const;

       /*----------------------------------------------------------------------------*/
       /**
        *  Accessor for setting the value of the back-slash escaping flag.
        *
        *  \param[in] state: the back-slash escaping flag value to be set.
        */
        void BackSlashEscapeState(bool state);

       /*----------------------------------------------------------------------------*/
       /**
        *   Accessor for retrieving the value of the error-abort flag.
        *   Default value for the error-abort flag is OFF.
        *
        *  \returns true if the error-abort flag is on; false otherwise.
        */
        bool ErrorAbortState() const;

       /*----------------------------------------------------------------------------*/
       /**
        *  Accessor for setting the value of the error-abort flag.
        *
        *  \param[in] state: the error-abort flag value to be set.
        */
        void ErrorAbortState(bool state);

       /*----------------------------------------------------------------------------*/
       /**
        *  Accessor for retrieving the search pattern.
        *
        *  \returns the search pattern string.
        */
        std::wstring GetPattern() const;

       /*----------------------------------------------------------------------------*/
       /**
        *  Performs file globbing with the given search pattern.
        */
        void DoGlob();

       /*----------------------------------------------------------------------------*/
       /**
        *  Check if the next matching file path exists or not. If exists, it advances its iterator to it.
        *
        *  \returns true if the next matching file path exists; otherwise returns false.
        */
        bool Next();

       /*----------------------------------------------------------------------------*/
       /**
        *  Retrieves the matching file path object currently pointed by the iterator 
		*  that goes over the result set of globbing.
        */
        SCXFilePath Current() const;

    private:
       /*----------------------------------------------------------------------------*/
       /**
        *  Copy Ctor - copying operation is unsupported.
        *
        *  \param[in] other: Ref to SCXGlob object to be copied.
        */      
        SCXGlob(SCXGlob &other);

       /*----------------------------------------------------------------------------*/
       /**
        *  Assignment Operator Overload - assignment operation is unsupported.
        *
        *  \param[in] rhs: Ref to SCXGlob object to be assigned.
        */
        SCXGlob& operator=(SCXGlob &rhs);

       /*----------------------------------------------------------------------------*/
       /**
        *  Private method for checking/altering a given search pattern before globbing.
        *  This method is called in SCXGlob constructors.
        *  If the search pattern is invalid, it throws SCXCoreLib::SCXInvalidArgumentException.
        */
        void NormalizePattern();

       /*----------------------------------------------------------------------------*/
       /** The search pattern for globbing. Will be on whatever multibyte format specified by LOCALE, normally UTF-8.
        */
        std::string m_pattern;

       /*----------------------------------------------------------------------------*/
       /** The pointer to an char * array containing matching file path names.
        */
        char **m_pathnames; 

       /*----------------------------------------------------------------------------*/
       /** The structure to hold the result of globbing.
        */
        glob_t m_globHolder;

       /*----------------------------------------------------------------------------*/
       /** The index for keeping track of the current position in iterating through the array of matching file path names.
        */
        int m_index;

       /*----------------------------------------------------------------------------*/
       /** The flag for specifying whether the back-slash escaping for the search pattern is on or off.
        */
        bool m_isBackSlashEscapeOn;

       /*----------------------------------------------------------------------------*/
       /** The flag for specifying whether the open or read directory error shall abort globbing.
        */
        bool m_isErrorAbortOn;

       /*----------------------------------------------------------------------------*/
       /** The handle to the log file.
        */
        SCXCoreLib::SCXLogHandle m_logHandle;
    };
}

#endif // SCXGLOB_H
