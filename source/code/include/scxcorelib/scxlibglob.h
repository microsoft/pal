/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
    \file scxlibglob.h

    \brief Library globbing interface sorted by filename version


 */
/*----------------------------------------------------------------------------*/

#ifndef SCXLIBGLOB_H
#define SCXLIBGLOB_H

// Due to strverscmp, this is only supported on linux platforms
#if defined(linux)

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfilepath.h>
#include <vector>
#include <string>

namespace SCXCoreLib
{
    class SCXLibGlob
    {
    private:
        std::wstring m_name; //!< Contains the glob string we will be searching for
        std::vector<std::wstring> m_paths; //!< Contains all of the paths we will be searching in
        
        /*----------------------------------------------------------------------------*/
        /**
           Validates filename is not null, has at least one '.', and that the '.' is not
           the last character in the filename.
           
           \param path : Path to file. 
           \returns true if path satifies all above conditions, false otherwise
        */
        bool IsGoodFileName(SCXCoreLib::SCXFilePath path);
        
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor
           \param name : The filename that will be SCXGlob'd for
           \param paths : The vector of paths that will be used to find "name"
        */
        SCXLibGlob(std::wstring name, std::vector<std::wstring> paths = std::vector<std::wstring>());
        
        /*----------------------------------------------------------------------------*/
        /**
           Does a series of SCXGlobs on each path specified in the constructor searching
           for the name specified in the constructor.
           \returns a vector of SCXFilePath (sorted by version, greatest first) of all 
           pathnames that match any paths/name combination.
        */
        std::vector<SCXCoreLib::SCXFilePath> Get();
    };
}
#endif // defined(linux)
#endif // SCXLIBGLOB_H
