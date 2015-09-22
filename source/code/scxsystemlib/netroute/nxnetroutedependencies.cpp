/*-----------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information

*/
/**
   \file nxnetroutedependencies.cpp

   \brief Dependencies needed by NxNetRoute classes

   \date  06-08-2015 16:20:00
 */
/*-----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/nxnetroutedependencies.h>
#include <sys/sysinfo.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilesystem.h>
#include <scxcorelib/scxstream.h>

using namespace SCXCoreLib;

namespace SCXSystemLib
{
    /**
       Constructor

       @detailed Allows you to pass in a new path for the file normally located in proc/net/route.
       @param[in] pathToRouteFile This is the full path to the route file.
    */
    NxNetRouteDependencies::NxNetRouteDependencies(std::wstring pathToProcNetRouteFile /* = L"/proc/net/route" */) : m_pathToProcNetRouteFile( pathToProcNetRouteFile )
    {
        m_log = SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.system.netroute.nxnetroutedependencies"));
        SCX_LOGTRACE(m_log, std::wstring(L"NxNetRouteDependencies constructor "));
    }

    /*-----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    NxNetRouteDependencies::~NxNetRouteDependencies()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteDependencies destructor");
    }

    /*-----------------------------------------------------------------------------*/
    /**
       Init - Reading in the file and populate the vector with the full lines, omitting
              the first line with the column headings.
    */
    void NxNetRouteDependencies::Init()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteDependencies Init()");

        // since there is nothing stopping init from being called
        // multiple times, ensure a clear slate before each call.
        m_lines.clear();

        // read from file
        SCXStream::NLFs nlfs;
        std::vector<std::wstring> lines;
        SCXFile::ReadAllLines(SCXFilePath(GetPathToFile()), lines, nlfs);

        if(lines.size() == 0)
        {
            SCX_LOGTRACE(m_log, L"NxNetRouteDependencies Init(): no lines found in file at " + GetPathToFile());
        }

        std::vector<std::wstring>::iterator iter;

        iter = lines.begin();// get iterator at beginnging of file
        iter++; // move past the first line with the labels

        while (iter != lines.end())
        {
            m_lines.push_back(*iter);
            ++iter;
        }
    }

    /*--------------------------------------------------------------------------------------------*/
    /**
       @return wstring The path to the route file.
    */
    std::wstring NxNetRouteDependencies::GetPathToFile() const
    {
        return m_pathToProcNetRouteFile;
    }

    /*--------------------------------------------------------------------------------------------*/
    /**
       @param[in] pathToProcNetRouteFile  Set the path to the route file as a wide string.
    */
    void NxNetRouteDependencies::SetPathToFile(const std::wstring pathToProcNetRouteFile)
    {
        m_pathToProcNetRouteFile = pathToProcNetRouteFile;
    }

    /*--------------------------------------------------------------------------------------------*/
    /**
       @return std::vector& A vector containing the lines of the route file
    */
    std::vector<std::wstring>& NxNetRouteDependencies::GetLines()
    {
        return m_lines;
    }
}
