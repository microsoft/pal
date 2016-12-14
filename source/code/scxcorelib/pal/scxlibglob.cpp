/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/

#if defined(linux)

#include <scxcorelib/scxglob.h>
#include <scxcorelib/scxlibglob.h>

#include <algorithm>
#include <vector>

using namespace std;

namespace SCXCoreLib
{
    SCXLibGlob::SCXLibGlob(wstring name, vector<wstring> paths)
        : m_name(name)
          , m_paths(paths)
    {
        // if directory paths are non-existant, provide default system library paths
        if (paths.size() == 0)
        {
#if PF_WIDTH == 64
            m_paths.push_back(L"/lib64");
            m_paths.push_back(L"/usr/lib64");
#endif          
            m_paths.push_back(L"/lib");
            m_paths.push_back(L"/usr/lib");
        }
    }
    
    bool SCXLibGlob::IsGoodFileName(SCXCoreLib::SCXFilePath path)
    {
        std::wstring str_filename = path.GetFilename(); 
        
        if (str_filename.length() == 0)
        {
            return false; 
        }
        
        size_t offset; 
        
        offset = str_filename.find_last_of('.');
        
        // No dots, there needs to be at least one
        if (offset == std::wstring::npos)
        {
            return false;
        }
        
        // Do not want dot at end of name
        if (offset == str_filename.length() - 1)
        {
            return false;
        }
        
        return true;
    }
    
    /*----------------------------------------------------------------------------*/
    /**
       Static comparator method for a sort routine. This compares the filenames of any
       two given paths, and determines which has a greater version number than the other by 
       using the "strverscmp" function (a GNU extension function).
       
       \param path1 : full path of a file
       \param path2 : full path of a file
       \return true if path1's filename is a greater version than path2's filename
    */
    static bool CompareVersions (SCXCoreLib::SCXFilePath path1, SCXCoreLib::SCXFilePath path2)
    {
        // if the path1's filename has a greater version than path2's filename, 
        // then put path1 before path2 in the sort.
        if (strverscmp(StrToUTF8(path1.GetFilename()).c_str(),
                       StrToUTF8(path2.GetFilename()).c_str()) > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    
    vector<SCXCoreLib::SCXFilePath> SCXLibGlob::Get()
    {
        SCXCoreLib::SCXFilePath dirpath;
        SCXCoreLib::SCXFilePath curfile;
        
        vector<SCXFilePath> matchedFiles;
        // check each directory for libs that match this
        for (vector<wstring>::iterator curDir = m_paths.begin(); curDir != m_paths.end(); curDir++)
        {
            dirpath.SetDirectory(*curDir);
            dirpath.SetFilename(wstring(m_name));
            SCXGlob glob(dirpath.Get());
            glob.DoGlob();
            while (glob.Next())
            {
                curfile = glob.Current();                
                
                // Validate for nice file names .. 
                if(IsGoodFileName(curfile))
                {
                    matchedFiles.push_back(curfile);
                }
            }
        }
        
        // at this point, matchedFiles contains all libraries that match.  Now let's sort it
        sort (matchedFiles.begin(), matchedFiles.end(), CompareVersions);

        return matchedFiles;
    }  
}

#endif // defined(linux)

