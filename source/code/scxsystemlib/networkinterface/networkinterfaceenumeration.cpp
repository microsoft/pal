/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implementation of network interface enumeration PAL
    
    \date        08-03-03 12:12:02

    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/networkinterfaceenumeration.h>
#include <scxsystemlib/networkinterface.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib {

/*----------------------------------------------------------------------------*/
/**
    Constructs an enumeration dependent on the actual system

   \param includeNonRunning - flag determines if all of the interfaces are to be returned or only interfaces that are
                              UP or RUNNING.
*/
NetworkInterfaceEnumeration::NetworkInterfaceEnumeration(bool includeNonRunning)
        : m_log(SCXLogHandleFactory::GetLogHandle(
                L"scx.core.common.pal.system.networkinterface.networkinterfaceenumeration")),
          m_deps(new NetworkInterfaceDependencies()),
          m_includeNonRunning(includeNonRunning)
{    
}

/*----------------------------------------------------------------------------*/
/**
    Constructs an enumeration dependent on injected dependencies rather than the actual system.
    Useful for testing behaviour in a reproducable manner.

   \param deps - dependencies. 
   \param includeNonRunning - flag determines if all of the interfaces are to be returned or only interfaces that are
                              UP or RUNNING. In the OM case we return only interfaces that are UP or RUNNING. In the
                              CM case we retrun all interfaces.
*/
NetworkInterfaceEnumeration::NetworkInterfaceEnumeration(
                SCXHandle<NetworkInterfaceDependencies> deps, bool includeNonRunning)
        : m_log(SCXLogHandleFactory::GetLogHandle(
                L"scx.core.common.pal.system.networkinterface.networkinterfaceenumeration")),
          m_deps(deps),
          m_includeNonRunning(includeNonRunning)
{    
}

/*----------------------------------------------------------------------------*/
//! Destructor
NetworkInterfaceEnumeration::~NetworkInterfaceEnumeration() {
}

/*----------------------------------------------------------------------------*/
//! Implementation of the Init method of the entity framework.
void NetworkInterfaceEnumeration::Init() {
    SCX_LOGTRACE(m_log, L"SCXSystemLib::NetworkInterfaceEnumeration::Init Calling UpdateEnumeration");

    UpdateEnumeration();
}

/*----------------------------------------------------------------------------*/
/**
   Implementation of the Update method of the entity framework.

   \param updateInstances - indicates whether only the existing instances shall be updated. 
   
   The method refreshes the set of known instances in the enumeration. 
   
   Any newly created instances must have a well-defined state after execution, 
   meaning that instances which update themselves have to init themselves upon 
   creation. 
*/
void NetworkInterfaceEnumeration::Update(bool updateInstances) {
    if (updateInstances) {
        UpdateInstances();        
    } else {
        UpdateEnumeration();
    }
    
}

/*----------------------------------------------------------------------------*/
//! Run the Update() method on all instances in the colletion, including the
//! Total instance if any.
//! \note Optimized implementation that recreates the same result as running update on
//!       each instance, but does not actually do so
void NetworkInterfaceEnumeration::UpdateInstances() {
    vector<NetworkInterfaceInfo> latestInterfaces = NetworkInterfaceInfo::FindAll(m_deps);
    typedef map<wstring, size_t> IndexByStrMap;
    IndexByStrMap latestInterfaceById;

    // Create an index of the latest instances by their id
    for (size_t nr = 0; nr < latestInterfaces.size(); nr++) {
        latestInterfaceById.insert(IndexByStrMap::value_type(NetworkInterfaceInstance(latestInterfaces[nr]).GetId(), nr));
    }    
    
    for (EntityIterator oldIter = Begin(); oldIter != End(); oldIter++) {
        IndexByStrMap::iterator stillExistingInterfacePos = latestInterfaceById.find((*oldIter)->GetId());        
        if (stillExistingInterfacePos != latestInterfaceById.end()) {                                    
            // Update instances that still exists
            (*oldIter)->Update(latestInterfaces[stillExistingInterfacePos->second]);
        }
    }
    
}

/*----------------------------------------------------------------------------*/
//! Make the enumeration correspond to the current state of the system
void NetworkInterfaceEnumeration::UpdateEnumeration() {
    SCX_LOGTRACE(m_log, L"SCXSystemLib::NetworkInterfaceEnumeration::UpdateEnumeration entry");

    SCX_LOGTRACE(m_log, L"SCXSystemLib::NetworkInterfaceEnumeration::UpdateEnumeration FindAll");
    vector<NetworkInterfaceInfo> latestInterfaces = NetworkInterfaceInfo::FindAll(m_deps, m_includeNonRunning);
    typedef map<wstring, size_t> IndexByStrMap;
    IndexByStrMap newInterfaceById;

    // Prepare an index of new instances by their id
    SCX_LOGTRACE(m_log, L"SCXSystemLib::NetworkInterfaceEnumeration::UpdateEnumeration Preparing indexes");
    for (size_t nr = 0; nr < latestInterfaces.size(); nr++) {
        newInterfaceById.insert(IndexByStrMap::value_type(NetworkInterfaceInstance(latestInterfaces[nr]).GetId(), nr));
    }    
    
    SCX_LOGTRACE(m_log, L"SCXSystemLib::NetworkInterfaceEnumeration::UpdateEnumeration Beginning loop");
    for (size_t nr = Size();  nr > 0; nr--) {
        SCX_LOGTRACE(m_log, L"SCXSystemLib::NetworkInterfaceEnumeration::UpdateEnumeration In loop: find");
        IndexByStrMap::iterator stillExistingInterfacePos = newInterfaceById.find(GetInstance(nr-1)->GetId());
        if (stillExistingInterfacePos != newInterfaceById.end()) {                                    
            // Update instances that still exists
            SCX_LOGTRACE(m_log, StrAppend(L"SCXSystemLib::NetworkInterfaceEnumeration::UpdateEnumeration Updating Interface ", GetInstance(nr-1)->GetId()));
            GetInstance(nr-1)->Update(latestInterfaces[stillExistingInterfacePos->second]);

            // Remove instances, that are not new, from index of new instances
            newInterfaceById.erase(stillExistingInterfacePos);
        } else { 
            // Remove instances, that no longer exists, from the enumeration
            SCX_LOGTRACE(m_log, StrAppend(L"SCXSystemLib::NetworkInterfaceEnumeration::UpdateEnumeration Removing Interface ", GetInstance(nr-1)->GetId()));
            RemoveInstance(Begin() + nr - 1);
        }
    }
    
    // Add new instances to the enumeration
    SCX_LOGTRACE(m_log, L"SCXSystemLib::NetworkInterfaceEnumeration::UpdateEnumeration Adding new instances ");
    for (IndexByStrMap::iterator iter = newInterfaceById.begin(); iter != newInterfaceById.end(); iter++) {
        const NetworkInterfaceInfo &intf = latestInterfaces[iter->second];
        bool knownState = intf.IsKnownIfUp() && intf.IsKnownIfRunning();
        bool acceptedAddress = !intf.IsIPAddressKnown() || intf.GetIPAddress().find(L"127.0.0.") != 0; 
        // Fix according to WI5275, don't add uninteresting interfaces
        if (knownState && acceptedAddress) {
            AddInstance(
                SCXCoreLib::SCXHandle<NetworkInterfaceInstance>(new NetworkInterfaceInstance(latestInterfaces[iter->second])));
        }
    }
}

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
