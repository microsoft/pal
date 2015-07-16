/*---------------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved. See license.txt for license information

*/
/**
   \file

   \brief      Enumeration of NetRoute

   \date       6-8-2015  

*/

#ifndef NXNETROUTEENUMERATION_H
#define NXNETROUTEENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/nxnetrouteinstance.h>
#include <scxsystemlib/nxnetroutedependencies.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxcmn.h>

using namespace SCXCoreLib;


namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------------*/
    /**
       Class that represents a collection of NetRoute

       PAL Holding collection of ComputerSystem

    */
    class NxNetRouteEnumeration: public EntityEnumeration<NxNetRouteInstance>
    {
        friend class NxNetRouteInstance;
        friend class NxNetRouteDependencies;

    public:
        virtual ~NxNetRouteEnumeration();
        NxNetRouteEnumeration(SCXCoreLib::SCXHandle<NxNetRouteDependencies> deps = SCXCoreLib::SCXHandle<NxNetRouteDependencies>(new NxNetRouteDependencies()));
        NxNetRouteEnumeration(SCXCoreLib::SCXHandle<NxNetRouteDependencies> deps, SCXCoreLib::SCXHandle<NxNetRouteInstance> inst);
        void AddNetRouteInstance(SCXCoreLib::SCXHandle<NxNetRouteInstance> instance);
        virtual void Init();
        virtual void Update(bool updateInstances = false);
        virtual void CleanUp();
        void writeToFile();
        std::wstring GetPathToFile();
        bool ValidateIface(std::wstring iface);
        bool ValidateNonRequiredParameters(std::wstring &param);
        void ValidateFlags(std::wstring &param);

    private:
        SCXCoreLib::SCXLogHandle m_log;
        SCXCoreLib::SCXHandle<NxNetRouteDependencies> m_deps;
        SCXCoreLib::SCXHandle<NxNetRouteInstance> m_inst;

    };
}
#endif /*NXNETROUTEENUMERATION_H*/
/*---------------------------------------------------------------------------------------*/
