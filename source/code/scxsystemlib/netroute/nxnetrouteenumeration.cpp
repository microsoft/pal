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

#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilesystem.h>

#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxregex.h>
#include <scxcorelib/scxip.h>

#include <cassert>
#include <sstream>
#include <ctype.h>

using namespace SCXSystemLib;

namespace SCXSystemLib
{
    /*------------------------------------------------------------------------------*/
    /**
       Constructor
       \detailed Contains the default path and acts as the default constructor if no value provided.
       \param[in] deps Dependency object for Net Route Enumeration.
    */
    NxNetRouteEnumeration::NxNetRouteEnumeration(SCXCoreLib::SCXHandle<NxNetRouteDependencies> deps /* = SCXCoreLib::SCXHandle<NxNetRouteDependencies>(new NxNetRouteDependencies()) */):
        EntityEnumeration<NxNetRouteInstance>(),
        m_deps(deps)
    {
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
       Init
    */
    void NxNetRouteEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration Init()");
    }

    /*------------------------------------------------------------------------------*/
    /**
       @param[in] SCXHandle<NxNetRouteInstance> An NxNetRouteInstance instance
       @detailed This method wraps the inherited AddInstance method, so that instances passed
                 in will be stored in the super classes' array
    */
    void NxNetRouteEnumeration::AddNetRouteInstance(SCXCoreLib::SCXHandle<NxNetRouteInstance> instance)
    {
        AddInstance(instance);
    }

    /*------------------------------------------------------------------------------*/
    /**
       Update
       /detailed This method parses an entire line of a route file and creates an
                 instance for each line which is then stored in an array.

       @param[in] updateInstances If true (the default) the file will be read.  If false, the
                       file will not be read and instead any pre-loaded testing lines
                       will instead be used.
       @throws SCXInternalErrorException If 11 elements are not parsed for each route file line.
    */
    void NxNetRouteEnumeration::Update(bool updateInstances /* = true */ )
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration Update()");

        if( updateInstances )
        {
            m_deps->Init(); // read in data from file
        }

        RemoveInstances();

        vector<std::wstring>::iterator iter;

        iter = m_deps->GetLines().begin();// get iterator at beginning of file

        while (iter != m_deps->GetLines().end())
        {
            std::wstring temp = *iter;
            std::vector<std::wstring> lineElements;
            StrTokenize(temp, lineElements, L"\t\n");

            if (lineElements.size() != 11)
            {
                std::wostringstream error;

                error << L"NxNetRouteEnumeration::Update expected 11 elements in line, got " << std::endl;
                error << lineElements.size() << "." << std::endl;
                throw SCXInternalErrorException(error.str(), SCXSRCLOCATION);
            }


            if (m_log.GetSeverityThreshold() <= SCXCoreLib::eTrace )
            {
                std::wostringstream error;

                error << L"NxNetRouteEnumeration::Update, parsing line of file:" << std::endl;
                error << temp;
                SCX_LOGTRACE( m_log, error.str() );
            }

            // create new instance
            SCXCoreLib::SCXHandle<NxNetRouteInstance> route;
            route = new NxNetRouteInstance(lineElements[0],
                                           lineElements[1],
                                           lineElements[2],
                                           lineElements[3],
                                           lineElements[4],
                                           lineElements[5],
                                           lineElements[6],
                                           lineElements[7],
                                           lineElements[8],
                                           lineElements[9],
                                           lineElements[10]);

            AddInstance(route);

            ++iter;// next line
        }

        SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration Update()");
    }

    /*------------------------------------------------------------------------------*/
    /**
       Write the instances to a route file
       /detailed Each instance becomes a line in the route file.  They are added to
                 file in the order they were added to this instance.  To make the resulting
                 file look exactly like a route file found on the computer, trailing spaces
                 are added to the end of each line.
    */
    void NxNetRouteEnumeration::Write()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration Write()");

        if(0 == Size())
        {
            SCX_LOGTRACE(m_log, L"NxNetRouteEnumeration Write called with nothing to write()");
            return;
        }

        std::vector<wstring>lines;
        lines.push_back(L"Iface\tDestination\tGateway \tFlags\tRefCnt\tUse\tMetric\tMask\t\tMTU\tWindow\tIRTT                                                       ");

        for(size_t i = 0; i < Size(); ++i)
        {
            SCXHandle<NxNetRouteInstance>instance = GetInstance(i);
            std::wostringstream oss;

            oss << instance->GetInterface() <<  L"\t";
            oss << SCXCoreLib::IP::ConvertIpAddressToHex(instance->GetDestination()) << L"\t";
            oss << SCXCoreLib::IP::ConvertIpAddressToHex(instance->GetGateway()) << L"\t";
            oss << instance->GetFlags() << L"\t";
            oss << instance->GetRefCount() << L"\t";
            oss << instance->GetUse() << L"\t";
            oss << instance->GetMetric() << L"\t";
            oss << SCXCoreLib::IP::ConvertIpAddressToHex(instance->GetGenMask()) << L"\t";
            oss << instance->GetMtu() << L"\t";
            oss << instance->GetWindow() << L"\t";
            oss << instance->GetIrtt();

            // each line should be 127 characters long with padding spaces at end
            oss << L"                                                                                                                           ";

            lines.push_back(oss.str().substr(0,127));
        }

        lines.push_back(L"");// add a blank line at end of file
        SCXFile::WriteAllLines(m_deps->GetPathToFile(), lines, std::ios_base::out);
    }

    /*------------------------------------------------------------------------------*/
    /**
       Validate the interface part of the route file

       @return bool Returns true if the wstring is a valid entry for the route file.
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
       @detailed This used in the NxNetRoute Provider code to ensure that the user
                 has supplied only valid digits for the fields that require only digits.
       @param[in] param A wstring which may contain alphanumeric characters
       @return bool Returns true if the parameter string consists only of numeric digits
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
    }
}

/*----------------------------------END-OF-FILE------------------------------------------*/
