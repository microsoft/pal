/*---------------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved. See license.txt for license information

*/
/**
   \file    nxnetrouteenumeration.h

   \brief   Enumeration of NetRoute

   \date    6-8-2015

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
 
    public:
        NxNetRouteEnumeration(SCXCoreLib::SCXHandle<NxNetRouteDependencies> deps = SCXCoreLib::SCXHandle<NxNetRouteDependencies>(new NxNetRouteDependencies()));
        virtual ~NxNetRouteEnumeration();
        void AddNetRouteInstance(SCXCoreLib::SCXHandle<NxNetRouteInstance> instance);
        virtual void Init();
        virtual void Update(bool updateInstances = true);
        virtual void CleanUp();
        bool ValidateIface(std::wstring iface);
        bool ValidateNonRequiredParameters(std::wstring &param);
        void ValidateFlags(std::wstring &param);
        void Write();

    private:
        SCXCoreLib::SCXLogHandle m_log; /*!< logging object */
        SCXCoreLib::SCXHandle<NxNetRouteDependencies> m_deps; /*!< The external dependencies, ie the path to the route file */

    };
}
#endif /*NXNETROUTEENUMERATION_H*/
/*---------------------------------------------------------------------------------------*/
