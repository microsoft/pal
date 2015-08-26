/*------------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information

*/
/**
   \file     nxnetrouteinstance.h

   \brief    Specification of net route instance

   \date     06-08-15   11:10:00

*/
/*----------------------------------------------------------------------------------*/
#ifndef NXNETROUTEINSTANCE_H
#define NXNETROUTEINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/nxnetroutedependencies.h>
//#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------------*/
    /**
       @brief An NxNetRoute instance
       @detailed One net route instance corresponds to one line in the file, the file is located at proc/net/route.
    */
    class NxNetRouteInstance: public EntityInstance
    {

    public:
        NxNetRouteInstance();
        NxNetRouteInstance(std::wstring &interface,
                           std::wstring &destination,
                           std::wstring &gateway,
                           std::wstring &flags,
                           std::wstring &refcount,
                           std::wstring &use,
                           std::wstring &metric,
                           std::wstring &genmask,
                           std::wstring &mtu,
                           std::wstring &window,
                           std::wstring &irtt);

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

        std::wstring m_interface;
        std::wstring m_destination;
        std::wstring m_gateway;
        std::wstring m_flags;
        std::wstring m_refcount;
        std::wstring m_use;
        std::wstring m_metric;
        std::wstring m_genmask;
        std::wstring m_mtu;
        std::wstring m_window;
        std::wstring m_irtt;

    protected:
        SCXCoreLib::SCXLogHandle m_log; /*!< logging object */

    private:
    };
}
#endif /*NXNETROUTEINSTANCE_H*/
