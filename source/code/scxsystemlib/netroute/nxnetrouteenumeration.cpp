/*-------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation.  All rights reserved. See license.txt license information

*/
/**
   \file     nxnetrouteenunmeration.cpp

   \brief    Enumeration of Net Route

   \date    2015-06-08 2:11:00

*/
/*------------------------------------------------------------------------------*/


#include <scxsystemlib/nxnetrouteenumeration.h>
#include <scxsystemlib/nxnetrouteinstance.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxcmn.h>
#include <arpa/inet.h>

#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilesystem.h>

#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxregex.h>
#include <scxcorelib/scxip.h>

#include <cassert>
#include <arpa/inet.h>
#include <sstream>
#include <ctype.h>

using namespace std;
using namespace SCXCoreLib;
using namespace SCXSystemLib;

namespace SCXSystemLib
{
    /*------------------------------------------------------------------------------*/
    /**
       Default Constructor

       \param[in] deps Dependenciea from Net Route Enumeration.
    */
    NxNetRouteEnumeration::NxNetRouteEnumeration(SCXCoreLib::SCXHandle<NxNetRouteDependencies> deps):
        EntityEnumeration<NxNetRouteInstance>(),
        m_deps(deps)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.netroute.nxnetrouteenumeration");
        SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration constructor");
    }

    NxNetRouteEnumeration::NxNetRouteEnumeration(SCXCoreLib::SCXHandle<NxNetRouteDependencies> deps, SCXCoreLib::SCXHandle<NxNetRouteInstance>inst)
    {
        m_deps = deps;
        m_inst = inst;
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.netroute.nxnetrouteenumeration");
        SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration constructor");
    }
    

    /*------------------------------------------------------------------------------*/
    /**
       Destructor
    */
    NxNetRouteEnumeration::~NxNetRouteEnumeration()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteEnumaration destructor");
    }

    /*------------------------------------------------------------------------------*/
    /**
       Create Net Route init
    */
    void NxNetRouteEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration Init()");
    }

    /*------------------------------------------------------------------------------*/
    /**
       Create Net Route instances
    */
    void NxNetRouteEnumeration::AddNetRouteInstance(SCXCoreLib::SCXHandle<NxNetRouteInstance> instance)
    {
        AddInstance(instance);
    }

    /*------------------------------------------------------------------------------*/
    /**
       Net Route Updates
    */
    void NxNetRouteEnumeration::Update(bool updateInstances)
    {
         vector<wstring>lines;
         vector<wstring>::iterator iter;

        if(updateInstances == true)
        {
            // use dependencies for testing
            lines = m_deps->vDependencyInjection;
            iter = lines.begin();
        }
        else
        {
            // read from file
            SCXStream::NLFs nlfs;
            std::wstring pathToFile = GetPathToFile();
            
            SCXFile::ReadAllLines(SCXFilePath(pathToFile), lines, nlfs);
            iter = lines.begin();
            iter++; // move past the first line with the labels
        }
       
        while (iter != lines.end())
        {
            // create new instance
            SCXCoreLib::SCXHandle<NxNetRouteInstance> route;
            route = new NxNetRouteInstance();
            
            
            std::wstring temp = *iter;
            std::vector<std::wstring> lineElements;
            StrTokenize(temp, lineElements, L"\t\n");

            route->m_interface = lineElements[0];
            route->m_destination = SCXCoreLib::IP::ConvertHexToIpAddress(lineElements[1]);
            route->m_gateway = SCXCoreLib::IP::ConvertHexToIpAddress(lineElements[2]);
            route->m_flags = lineElements[3];
            route->m_refcount = lineElements[4];
            route->m_use = lineElements[5];
            route->m_metric = lineElements[6];
            route->m_genMask = SCXCoreLib::IP::ConvertHexToIpAddress(lineElements[7]);
            route->m_mtu = lineElements[8];
            route->m_window = lineElements[9];
            route->m_irtt = lineElements[10];

            AddInstance(route);

            ++iter;
        }
        
        SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration Update()");
    }

    /*------------------------------------------------------------------------------*/
    /**
       Return the file path for netroute
    */

    std::wstring NxNetRouteEnumeration::GetPathToFile()
    {
         return m_deps->GetPathToFile();
    }

    /*------------------------------------------------------------------------------*/
    /**
       Validate the interface
    */
 
     bool NxNetRouteEnumeration::ValidateIface(std::wstring iface)
     {
         if (iface.compare(L"lo") == 0 )
             return true;

        SCXRegex re(L"^eth[0-9][0-9]?$");

        return  re.IsMatch(iface);
     }
    
    /*------------------------------------------------------------------------------*/
    /**
       Validate the non required parameters
    */

     bool NxNetRouteEnumeration::ValidateNonRequiredParameters(std::wstring &param)
     {
         if (param.empty())
         {
             param = L"0";
         }
         else
         {
             for(uint i = 0; i < param.length(); i++)
             {
                 if(!isdigit(param[i]))
                 {
                     return false;
                 }
             }
         }
         return true; 
     }
    /*------------------------------------------------------------------------------*/
    /**
       Cleanup
    */
    void NxNetRouteEnumeration::CleanUp()
    {
        m_deps->CleanUp();
    }
}

/*----------------------------------END-OF-FILE------------------------------------------*/
