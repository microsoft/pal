/*------------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information

*/
/**
   \file

   \bried    Specification of net route instance

   \date     06-08-15   11:10:00

*/
/*----------------------------------------------------------------------------------*/
#ifndef NXNETROUTEINSTANCE_H
#define NXNETROUTEINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/nxnetroutedependencies.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------------*/
    /**
       Represent net route instance
    */
    class NxNetRouteInstance: public EntityInstance
    {
        friend class NxNetRouteEnumeration;

    public:
        NxNetRouteInstance();
        NxNetRouteInstance(SCXCoreLib::SCXHandle<NxNetRouteDependencies> deps);
        ~NxNetRouteInstance();
        void Update();
        std::wstring GetDestination();
        std::wstring GetGenMask();
        std::wstring GetGateway();
        std::wstring GetInterface();
        std::wstring GetFlags();
        std::wstring GetRefCount();
        std::wstring GetUse();
        std::wstring GetMetric();
        std::wstring GetMtu();
        std::wstring GetWindow();
        std::wstring GetIrtt();
        



        std::wstring m_destination;
        std::wstring m_gateway;
        std::wstring m_interface;
        std::wstring m_flags;
        std::wstring m_refcount;
        std::wstring m_use;
        std::wstring m_metric;
        std::wstring m_genMask;
        std::wstring m_mtu;
        std::wstring m_window;
        std::wstring m_irtt;

        void SetInterface(std::wstring interface);
        void SetDestination(std::wstring destination);        



    protected:

        // some objects seem to have these in various objects, doesn't seem to be one right place.
        SCXCoreLib::SCXHandle<NxNetRouteDependencies> m_deps;
        SCXCoreLib::SCXLogHandle m_log;

    private:




    };
}
#endif /*NXNETROUTEINSTANCE_H*/
