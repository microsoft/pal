/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file
 
    \brief     Implements the globbing PAL
 
    \date      08-05-22 16:05:00
 

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxglob.h>

#include <errno.h>

using namespace std;


// Note: The original SCXGlob implementation was done without use of SCXFilePath
//       objects. Instead wstring was used for pattern everywhere. This is still
//       reflected in these tests, but when adding more tests, do use the 
//       SCXFilePath variants exclusively.

namespace SCXCoreLib
{
        /** Constant used within the SCXGlobbing implementation. */
    static const char folderSeparator = 
    #ifdef WIN32
        '\\'
    #else
        '/'
    #endif
    ;

        const int cNoData = -2;  //!< Constant for index denoting instance state being no valid globbing results
        const int cBegin = -1;   //!< Constant for index denoting instance state being at beginning of globbing results

        SCXGlob::SCXGlob(const wstring &pttrn) :
                m_pathnames(NULL), m_index(cNoData), m_isBackSlashEscapeOn(true), m_isErrorAbortOn(false)
        {
                if (L"" == pttrn)
                {
                        throw SCXInvalidArgumentException(L"pattern", L"Empty pattern not allowed", SCXSRCLOCATION);
                }

                memset(&m_globHolder, 0, sizeof(glob_t));
                
                this->m_logHandle = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.os.scxglob");
                this->m_pattern = StrToUTF8(pttrn);
                this->NormalizePattern();
        }
        
    SCXGlob::SCXGlob(const SCXFilePath &pttrn) :
                m_pathnames(NULL), m_index(cNoData), m_isBackSlashEscapeOn(true), m_isErrorAbortOn(false)
    {
        if (L"" == pttrn.Get())
        {
            throw SCXInvalidArgumentException(L"pattern", L"Empty pattern not allowed", SCXSRCLOCATION);
        }

                memset(&m_globHolder, 0, sizeof(glob_t));

        this->m_logHandle = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.os.scxglob");
        this->m_pattern = StrToUTF8(pttrn.Get());
        this->NormalizePattern();
    }

    SCXGlob::~SCXGlob()
    {
        SCX_LOGTRACE(m_logHandle, L"~SCXGlob()");
        if (this->m_pathnames)
        {
            globfree(&this->m_globHolder);
                        memset(&m_globHolder, 0, sizeof(glob_t));
        }
    }

    bool SCXGlob::BackSlashEscapeState() const
    {
        SCX_LOGTRACE(m_logHandle, L"BackSlashEscapeState()");
        return this->m_isBackSlashEscapeOn;
    }

    void SCXGlob::BackSlashEscapeState(bool state)
    {
        SCX_LOGTRACE(m_logHandle, L"BackSlashEscapeState(bool state)");
        this->m_isBackSlashEscapeOn = state;
    }

    bool SCXGlob::ErrorAbortState() const
    {
        SCX_LOGTRACE(m_logHandle, L"ErrorAbortState()");
        return this->m_isErrorAbortOn;
    }

    void SCXGlob::ErrorAbortState(bool state)
    {
        SCX_LOGTRACE(m_logHandle, L"ErrorAbortState(bool state)");
        this->m_isErrorAbortOn = state;
    }

    wstring SCXGlob::GetPattern() const
    {
        SCX_LOGTRACE(m_logHandle, L"GetPattern()");
        return StrFromUTF8(this->m_pattern);
    }

    void SCXGlob::DoGlob()
    {
        SCX_LOGTRACE(m_logHandle, L"Initialize()");

                // Set a default 
                m_index = cNoData;

        if (this->m_pathnames)
        {
            globfree(&this->m_globHolder);
                        memset(&m_globHolder, 0, sizeof(glob_t));
            this->m_pathnames = 0;
        }

        // Prepares the flag to pass to the OS-provided glob().
        int flag = 0;
        if (!(this->m_isBackSlashEscapeOn))
        {
            flag |= GLOB_NOESCAPE;
        }       

        if (this->m_isErrorAbortOn)
        {
            flag |= GLOB_ERR;
        }

        // Calls the OS-provided glob().
        int ret = glob(m_pattern.c_str(), flag, NULL, &this->m_globHolder);

        switch (ret)
        {
        case 0:
            this->m_pathnames = this->m_globHolder.gl_pathv;
                        m_index = cBegin;
            break;
        case GLOB_NOMATCH:
            // No matching pathname exists in the file system.
            // Does nothing here.
            break;
        case GLOB_NOSPACE:
        {
            globfree(&this->m_globHolder);
            wstring message = L"SCXGlob_NoSpace_Error: " + StrFromUTF8(this->m_pattern);
            throw SCXResourceExhaustedException(L"Memory", message, SCXSRCLOCATION);
        }
        case GLOB_ABORTED:
        {
            // We are here only if isErrorAbortOn is true.
            globfree(&this->m_globHolder);
            wstring message = L"SCXGlob::Initialize(): " + StrFromUTF8(this->m_pattern);
            throw SCXErrnoException(message, errno, SCXSRCLOCATION);
        }
        default:
            globfree(&this->m_globHolder);
            wstring message = L"SCXGlob_Unknown_Error: " + StrFromUTF8(this->m_pattern);
            throw SCXInternalErrorException(message, SCXSRCLOCATION);
        }  
    }


    bool SCXGlob::Next()
    {
        SCX_LOGTRACE(m_logHandle, L"Next()");
                
                if (cNoData == this->m_index)
                {
                        return false;
                }

        bool hasMore = false;
        if (0 < this->m_globHolder.gl_pathc)
        {
            while (this->m_pathnames[++m_index])
            {
                // Excludes ./ and ../
                size_t len = strlen(this->m_pathnames[m_index]);
                if (!('.' == this->m_pathnames[m_index][len-1] && (1 == len || folderSeparator == this->m_pathnames[m_index][len-2]) ) &&
                    !('.' == this->m_pathnames[m_index][len-1] && '.' == this->m_pathnames[m_index][len-2] &&
                     (2 == len || folderSeparator == this->m_pathnames[m_index][len-3]) ) )
                {
                    hasMore = true;
                    break;
                }
            }
        }
                if (!hasMore)
                {
                        // If no more, remain Current at last item
                        m_index--;
                }
        return hasMore;
    }

    SCXFilePath SCXGlob::Current() const
    {
        SCX_LOGTRACE(m_logHandle, L"Current()");
                
                if(cNoData == m_index)
                {
                        return SCXFilePath();
                }
                
                if (this->m_pathnames[m_index]) {
                        return StrFromUTF8(this->m_pathnames[m_index]);
                } 
                else 
                {
                        return SCXFilePath();
                }
    }

    void SCXGlob::NormalizePattern()
    {
        SCX_LOGTRACE(m_logHandle, L"NormalizePattern()");

        string separator(1, folderSeparator);

#ifndef WIN32
        // Throws an exception if the pattern does not start with a separator character.
        if (0 != separator.compare(0, 1, this->m_pattern, 0, 1))
        {
            throw SCXInvalidArgumentException(L"pattern", L"Pattern must start with a directory separator", SCXSRCLOCATION);
        }

        // Throws an exception if found a relative path.
        // The ls command supports relative paths -- commenting out the following check
        // in order to emulate the ls command's behaviour.
        // string dot(1, '.');
        /*if (string::npos != this->m_pattern.find(dot+separator) || // "./"
            string::npos != this->m_pattern.find(dot+dot+separator)) // "../"
        {
            throw SCXInvalidArgumentException(L"pattern", L"No relative path allowed", SCXSRCLOCATION);
        }*/

#endif

        // Removes the trailing slash(es) if any.
        size_t len = this->m_pattern.size() - 1;
        while (0 < len && folderSeparator == this->m_pattern[len])
        {
            len--;
        }

        if (len < this->m_pattern.size() - 1)
        {
            this->m_pattern = this->m_pattern.erase(len+1);
        }
    }
}
