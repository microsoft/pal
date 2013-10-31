/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implementation of network interface instance PAL
    
    \date        08-03-03 12:12:02

    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/networkinterfaceinstance.h>
#include <scxsystemlib/networkinterface.h>
#include <vector>

using namespace std;

namespace SCXSystemLib {

/*----------------------------------------------------------------------------*/
//! Constructor
//! \param[in]   info     Initial information
NetworkInterfaceInstance::NetworkInterfaceInstance(const NetworkInterfaceInfo &info) 
        : EntityInstance(info.GetName()), m_info(new NetworkInterfaceInfo(info)) {
    
}

/*----------------------------------------------------------------------------*/
//! Destructor
NetworkInterfaceInstance::~NetworkInterfaceInstance() {
}

/*----------------------------------------------------------------------------*/
//! Name of interface
//! \returns Name
wstring NetworkInterfaceInstance::GetName() const {
    return m_info->GetName();
}

/*----------------------------------------------------------------------------*/
//!  \copydoc SCXSystemLib::EntityInstance::Update()
void NetworkInterfaceInstance::Update() {
    m_info->Refresh();
}

/*----------------------------------------------------------------------------*/
//! Make the content correspond to given information
//! \param[in]   info    New content
void NetworkInterfaceInstance::Update(const NetworkInterfaceInfo &info) {
    (*m_info) = info; 
}

/*----------------------------------------------------------------------------*/
//! IP Address assigned to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetIPAddress(wstring &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eIPAddress)) {
        value = m_info->GetIPAddress();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! IP Address assigned to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetIPAddress(vector<wstring> &value) const {
    value.clear();
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eIPAddress))
    {
        value.push_back(m_info->GetIPAddress());
    }
    vector<wstring> tmpIPv6 = m_info->GetIPV6Address();
    value.insert(value.end(), tmpIPv6.begin(), tmpIPv6.end());
    return !value.empty();
}

/*----------------------------------------------------------------------------*/
//! Netmask assigned to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetNetmask(wstring &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eNetmask)) {
        value = m_info->GetNetmask();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Broadcast address assigned to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetBroadcastAddress(wstring &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eBroadcastAddress)) {
        value = m_info->GetBroadcastAddress();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of bytes received from interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetBytesReceived(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eBytesReceived)) {
        value = m_info->GetBytesReceived();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of bytes sent to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetBytesSent(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eBytesSent)) {
        value = m_info->GetBytesSent();
        supported = true;
    }
    return supported;
}
        
/*----------------------------------------------------------------------------*/
//! Number of packets received from interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetPacketsReceived(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::ePacketsReceived)) {
        value = m_info->GetPacketsReceived();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of packets sent to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetPacketsSent(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::ePacketsSent)) {
        value = m_info->GetPacketsSent();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of errors that have occurred when receiving from interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetErrorsReceiving(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eErrorsReceiving)) {
        value = m_info->GetErrorsReceiving();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of errors that have occurred when sending to interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetErrorsSending(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eErrorsSending)) {
        value = m_info->GetErrorsSending();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Number of collisons that have occurred on interface
//! \param[out] value     Reference to value of property to be initialized
//! \returns    true iff property is supported
bool NetworkInterfaceInstance::GetCollisions(scxulong &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eCollisions)) {
        value = m_info->GetCollisions();
        supported = true;
    }
    return supported;
}

/*----------------------------------------------------------------------------*/
//! Is the interface up
//! \returns true iff if up
bool NetworkInterfaceInstance::GetUp(bool &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eUp)) {
        value =  m_info->IsUp();
        supported = true;
    }
    return supported;
}


/*----------------------------------------------------------------------------*/
//! Is the interface running
//! \returns true iff if running
bool NetworkInterfaceInstance::GetRunning(bool &value) const {
    bool supported = false;
    if (m_info->IsValueKnown(NetworkInterfaceInfo::eRunning)) {
        value = m_info->IsRunning();
        supported = true;
    }
    return supported;
}

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve availability and status of the device.

        \param       value - output parameter where the availability of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetAvailability(unsigned short& value) const
    {
        return m_info->GetAvailability(value);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve network medium in use of the device.

        \param       value - output parameter where the AdapterType of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetAdapterType(wstring& value) const
    {
        return m_info->GetAdapterType(value);
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve network medium ID in use of the device.

        \param       value - output parameter where the AdapterTypeID of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetAdapterTypeID(unsigned short& value) const
    {
        return m_info->GetAdapterTypeID(value);
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve if the network adapter can automatically determine the speed of the attached or network media.

        \param       value - output parameter where the AutoSense of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetAutoSense(bool& value) const
    {
        return m_info->GetAutoSense(value);
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve index value that uniquely identifies the local network interface of the device.

        \param       value - output parameter where the InterfaceIndex of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetInterfaceIndex(unsigned int& value) const
    {
        return m_info->GetInterfaceIndex(value);
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve media access control address for this network adapter of the device.

        \param value - output parameter where the MACAddress of the device is stored.
        \param sepChar - the delimiter.
        \param upperCase - true for uppercase and false to preserve the case.
        \returns true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetMACAddress(wstring& value, 
                                                 const char sepChar/* =':' */,
                                                 bool upperCase/* =false */) const
    {
        return m_info->GetMACAddress(value, sepChar, upperCase);
    }

    /*----------------------------------------------------------------------------*/
    /**
      Retrieve the raw form of the media access control address for this network 
      adapter of the device where the case is preserved and there is no delimiter.

      \param value - output parameter where the MACAddress of the device is stored.
      \returns true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetMACAddressRAW(std::wstring& value) const
    {
        return m_info->GetMACAddressRAW(value);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve maximum speed, in bits per second, for the network adapter.

        \param       value - output parameter where the MaxSpeed of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetMaxSpeed(scxulong& value) const
    {
        return m_info->GetMaxSpeed(value);
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve state of the network adapter connection to the network.

        \param       value - output parameter where the NetConnectionStatus of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetNetConnectionStatus(unsigned short& value) const
    {
        return m_info->GetNetConnectionStatus(value);
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve whether the adapter is a physical or a logical adapter.

        \param       value - output parameter where the PhysicalAdapter of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetPhysicalAdapter(bool& value) const
    {
        return m_info->GetPhysicalAdapter(value);
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve estimate of the current bandwidth in bits per second.

        \param       value - output parameter where the Speed of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInstance::GetSpeed(scxulong& value) const
    {
        return m_info->GetSpeed(value);
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
