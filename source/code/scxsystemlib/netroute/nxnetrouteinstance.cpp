/*-------------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All right reserved. See licinse.txt for license inforamtion
*/
/**
   \file nxnetrouteinstance.cpp

   \brief Instances used by NxNetRoute classes

   \date 06-08-2015   17:20:00

 */
/*-------------------------------------------------------------------------------------*/
#include <scxsystemlib/nxnetrouteinstance.h>

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
   Constructor with dependecies
*/
NxNetRouteInstance::NxNetRouteInstance(SCXCoreLib::SCXHandle<NxNetRouteDependencies> deps)
{
    m_deps = deps;
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
   Set Interface variable
*/
void NxNetRouteInstance::SetInterface(std::wstring interface) 
{
    m_interface = interface; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Set Destination  variable
*/
void NxNetRouteInstance::SetDestination(std::wstring destination)
{
    m_destination= destination; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return destination
*/
std::wstring NxNetRouteInstance::GetDestination()
{ 
    return m_destination; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return mask
*/
std::wstring NxNetRouteInstance::GetGenMask() 
{ 
    return m_genMask; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return gateway
*/
std::wstring NxNetRouteInstance::GetGateway() 
{ 
    return m_gateway; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return flags
*/
std::wstring NxNetRouteInstance::GetFlags() 
{ 
    return m_flags; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return referenceCount
*/
std::wstring NxNetRouteInstance::GetRefCount() 
{ 
    return m_refcount; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return interface
*/
std::wstring NxNetRouteInstance::GetInterface() 
{ 
    return m_interface; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return use
*/
std::wstring NxNetRouteInstance::GetUse() 
{ 
    return m_use; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return metric
*/
std::wstring NxNetRouteInstance::GetMetric() 
{ 
    return m_metric; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return MTU
*/
std::wstring NxNetRouteInstance::GetMtu() 
{ 
    return m_mtu; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return window
*/
std::wstring NxNetRouteInstance::GetWindow() 
{ 
    return m_window; 
}

/*-------------------------------------------------------------------------------------*/
/**
   Return IRTT
*/
std::wstring NxNetRouteInstance::GetIrtt() 
{ 
    return m_irtt; 
}
