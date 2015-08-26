/*-------------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All right reserved. See license.txt for license inforamtion
*/
/**
   \file nxnetrouteinstance.cpp

   \brief Each instance corresponds to one line in the proc/net/route file.
   \detailed A net route instance will only hold one line item of a route table file.  For one
       /proc/net/route file, you will probabaly need two or more instances to hold the whole file's
       worth of routes.  At this point in time, the routing files are identical between suse, debian,
       and rhel6.

   \date 06-08-2015   17:20:00

 */
/*-------------------------------------------------------------------------------------*/
#include <scxsystemlib/nxnetrouteinstance.h>
#include <scxcorelib/scxip.h>

using namespace SCXSystemLib;

/*-------------------------------------------------------------------------------------*/
/**
   Constructor
*/
NxNetRouteInstance::NxNetRouteInstance()
{
}

/*-------------------------------------------------------------------------------------*/
/**
   Constructor
   @param[in] interface The route interface.
   @param[in] destination The destination passed as hex format.
   @param[in] gateway The route gateway passed as hex format.
   @param[in] flags The route flags.
   @param[in] use The route use.
   @param[in] metric The route metric.
   @param[in] genmask The route genmask passed in as hex format.
   @param[in] mtu The route maximum transmission units.
   @param[in] window The route window.
   @param[in] irtt The route inital round trip time.
*/
NxNetRouteInstance::NxNetRouteInstance(std::wstring &interface,
                                       std::wstring &destination,
                                       std::wstring &gateway,
                                       std::wstring &flags,
                                       std::wstring &refcount,
                                       std::wstring &use,
                                       std::wstring &metric,
                                       std::wstring &genmask,
                                       std::wstring &mtu,
                                       std::wstring &window,
                                       std::wstring &irtt)  : m_interface(interface),
                                                              m_destination(SCXCoreLib::IP::ConvertHexToIpAddress(destination)),
                                                              m_gateway(SCXCoreLib::IP::ConvertHexToIpAddress(gateway)),
                                                              m_flags(flags),
                                                              m_refcount(refcount),
                                                              m_use(use),
                                                              m_metric(metric),
                                                              m_genmask(SCXCoreLib::IP::ConvertHexToIpAddress(genmask)),
                                                              m_mtu(mtu),
                                                              m_window(window),
                                                              m_irtt(irtt)
{
    if (m_log.GetSeverityThreshold() <= SCXCoreLib::eTrace )
    {
        std::wostringstream error;

        error << L"NxNetRouteInstance Constructor input values:" << std::endl;
        error << L"Interface:" << interface  << "( " << m_interface <<  " )" <<  std::endl;
        error << L"Destination:" << destination << "( " << m_destination <<  " )" <<  std::endl;
        error << L"Gateway:" << gateway << std::endl;
        error << L"Flags:" << flags << std::endl;
        error << L"RefCount:" << refcount << std::endl;
        error << L"Use:" << use << std::endl;
        error << L"Metric:" << metric << std::endl;
        error << L"GenMask:" << genmask  << "( " << m_genmask <<  " )"  << std::endl;
        error << L"MTU:" << mtu << std::endl;
        error << L"Window:" << window << std::endl;
        error << L"IRTT:" << irtt << std::endl;

        SCX_LOGTRACE( m_log, error.str() );
    }
}

/*-------------------------------------------------------------------------------------*/
/**
   Destructor
*/
NxNetRouteInstance::~NxNetRouteInstance() {}


/*-------------------------------------------------------------------------------------*/
/**
   Update
*/
void NxNetRouteInstance::Update()
{
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the destination part of the route.
   @return wstring The destination part of a route.
*/
std::wstring NxNetRouteInstance::GetDestination()
{
    return m_destination;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief holds the genmask/net mask part of the route.
   @return wstring The Mask part of a route.
*/
std::wstring NxNetRouteInstance::GetGenMask()
{
    return m_genmask;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the gateway part of the route.
   @return wstring The gateway part of a route.
*/
std::wstring NxNetRouteInstance::GetGateway()
{
    return m_gateway;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the flags part of the route.
   @return wstring The flags part of a route.
*/
std::wstring NxNetRouteInstance::GetFlags()
{
    return m_flags;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the reference count part of the route.
   @return wstring The referenceCount part of a route.
*/
std::wstring NxNetRouteInstance::GetRefCount()
{
    return m_refcount;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the interface part of the route.
   @return wstring The interface part of a route.
*/
std::wstring NxNetRouteInstance::GetInterface()
{
    return m_interface;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the Use part of the route.
   @return wstring The Use part of the route.
*/
std::wstring NxNetRouteInstance::GetUse()
{
    return m_use;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the metric portion of the route.
   @return wstring The metric part of a route.
*/
std::wstring NxNetRouteInstance::GetMetric()
{
    return m_metric;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the Maximum Transmission Units part of the route.
   @return wstring The Maximum Transmission Units.
*/
std::wstring NxNetRouteInstance::GetMtu()
{
    return m_mtu;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Gets the window part of a route.
   @return wstring The window part of a route.
*/
std::wstring NxNetRouteInstance::GetWindow()
{
    return m_window;
}

/*-------------------------------------------------------------------------------------*/
/**
   @brief Get the initial round trip time of a route.
   @return wstring The initial round trip time (irtt) part of a route.
*/
std::wstring NxNetRouteInstance::GetIrtt()
{
    return m_irtt;
}
