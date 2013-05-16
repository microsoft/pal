/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Simple warning message implementation for unit tests.
    
    \date        2008-03-18 11:16:00

    Only one includer (prefereably the consumer of warning messages should define 
    IMPLEMENT_SCXUNIT_WARNING before including this file.

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXUNITWARNING_H
#define SCXUNITWARNING_H

#include <string>
#include <vector>
#include <sstream>

namespace SCXCoreLib
{
/*----------------------------------------------------------------------------*/
/**
    Static class providing methods to store warning messages.
*/
    class SCXUnitWarning
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Adds a warning message to the warning list.

           \param message Warning message to add.
        */
        static void AddWarning(const std::wstring& message)
        {
            s_warnings.push_back(message);
        }

        /*----------------------------------------------------------------------------*/
        /**
           Adds a warning message to the warning list including line and file information.

           \param message Warning message to add.
           \param file name of file where warning occurs.
           \param line line number in given file.

           This method is used by the SCXUNIT_WARNING macro.
        */

        static void AddWarning(const std::wstring& message, const char* file, int line)
        {
            std::wstringstream msg;
            msg << file << ":" << line << " - " << message;
            AddWarning(msg.str());
        }

        /*----------------------------------------------------------------------------*/
        /**
           Returns the oldest warning message.
           
           \returns The oldest warning message added and not yet popped. An empty 
           string is returned if no warning message is available.
        */
        static std::wstring PopWarning()
        {
            if (s_warnings.begin() == s_warnings.end())
            {
                return L"";
            }
            std::wstring msg = s_warnings.front();
            s_warnings.erase(s_warnings.begin());
            return msg;
        }

    private:
        static std::vector<std::wstring> s_warnings; //!< list of warning messages.

        SCXUnitWarning() { }; //!< Constructor private since class should only be used as a static class.
    };

#if defined(IMPLEMENT_SCXUNIT_WARNING)
    std::vector<std::wstring> SCXUnitWarning::s_warnings;
#endif
    
} /* namespace SCXCoreLib */

#define SCXUNIT_WARNING(msg) SCXCoreLib::SCXUnitWarning::AddWarning(msg,__FILE__,__LINE__)

#endif /* SCXUNITWARNING_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
