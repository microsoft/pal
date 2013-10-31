/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file        networkinterfaceconfigurationinstance.cpp

\brief       Implementation of network interface configuration instance PAL

\date        12-30-12 12:12:02


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/networkinterfaceconfigurationinstance.h>
#include <vector>

using namespace std;

namespace SCXSystemLib {
    
    /*----------------------------------------------------------------------------*/
    //! Constructor
    //! \param[in]   info     Initial information
    NetworkInterfaceConfigurationInstance::NetworkInterfaceConfigurationInstance(const NetworkInterfaceInstance &instance) 
    : EntityInstance(instance.GetName())
    {
        for (int i = 0; i < 61; i++)
        {
            m_knownAttributesBitset[i] = false;
        }
    }
    
    /*----------------------------------------------------------------------------*/
    //! Destructor
    NetworkInterfaceConfigurationInstance::~NetworkInterfaceConfigurationInstance()
    {
    }
    
    /*----------------------------------------------------------------------------*/
    //! Name of interface
    //! \returns Name
    wstring NetworkInterfaceConfigurationInstance::GetName() const
    {
        return GetId();
    }
    
    /*----------------------------------------------------------------------------*/
    //! ArpAlwaysSourceRoute assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetArpAlwaysSourceRoute(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eArpAlwaysSourceRoute))
        {
            value = m_ArpAlwaysSourceRoute;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! ArpUseEtherSNAP assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetArpUseEtherSNAP(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eArpUseEtherSNAP))
        {
            value = m_ArpUseEtherSNAP;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! Caption assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetCaption(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eCaption))
        {
            value = m_Caption;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DatabasePath assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDatabasePath(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDatabasePath))
        {
            value = m_DatabasePath;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DeadGWDetectEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDeadGWDetectEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDeadGWDetectEnabled))
        {
            value = m_DeadGWDetectEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DefaultIPGateway assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDefaultIPGateway(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDefaultIPGateway))
        {
            value = m_DefaultIPGateway;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DefaultTOS assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDefaultTOS(Uint8 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDefaultTOS))
        {
            value = m_DefaultTOS;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DefaultTTL assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDefaultTTL(Uint8 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDefaultTTL))
        {
            value = m_DefaultTTL;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! Description assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDescription(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDescription))
        {
            value = m_Description;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DHCPEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDHCPEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDHCPEnabled))
        {
            value = m_DHCPEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DHCPLeaseExpires assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDHCPLeaseExpires(SCXCalendarTime &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDHCPLeaseExpires))
        {
            value = m_DHCPLeaseExpires;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DHCPLeaseObtained assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDHCPLeaseObtained(SCXCalendarTime &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDHCPLeaseObtained))
        {
            value = m_DHCPLeaseObtained;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DHCPServer assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDHCPServer(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDHCPServer))
        {
            value = m_DHCPServer;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DNSDomain assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDNSDomain(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDNSDomain))
        {
            value = m_DNSDomain;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DNSDomainSuffixSearchOrder assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDNSDomainSuffixSearchOrder(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDNSDomainSuffixSearchOrder))
        {
            value = m_DNSDomainSuffixSearchOrder;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DNSEnabledForWINSResolution assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDNSEnabledForWINSResolution(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDNSEnabledForWINSResolution))
        {
            value = m_DNSEnabledForWINSResolution;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DNSHostName assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDNSHostName(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDNSHostName))
        {
            value = m_DNSHostName;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DNSServerSearchOrder assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDNSServerSearchOrder(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDNSServerSearchOrder))
        {
            value = m_DNSServerSearchOrder;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! DomainDNSRegistrationEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetDomainDNSRegistrationEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eDomainDNSRegistrationEnabled))
        {
            value = m_DomainDNSRegistrationEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! ForwardBufferMemory assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetForwardBufferMemory(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eForwardBufferMemory))
        {
            value = m_ForwardBufferMemory;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! FullDNSRegistrationEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetFullDNSRegistrationEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eFullDNSRegistrationEnabled))
        {
            value = m_FullDNSRegistrationEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! GatewayCostMetric assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetGatewayCostMetric(vector<Uint16> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eGatewayCostMetric))
        {
            value = m_GatewayCostMetric;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IGMPLevel assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIGMPLevel(Uint8 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIGMPLevel))
        {
            value = m_IGMPLevel;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! Index assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIndex(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIndex))
        {
            value = m_Index;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! InterfaceIndex assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetInterfaceIndex(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eInterfaceIndex))
        {
            value = m_InterfaceIndex;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPAddress assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPAddress(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPAddress))
        {
            value = m_IPAddress;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPConnectionMetric assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPConnectionMetric(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPConnectionMetric))
        {
            value = m_IPConnectionMetric;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPEnabled))
        {
            value = m_IPEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPFilterSecurityEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPFilterSecurityEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPFilterSecurityEnabled))
        {
            value = m_IPFilterSecurityEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPPortSecurityEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPPortSecurityEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPPortSecurityEnabled))
        {
            value = m_IPPortSecurityEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPSecPermitIPProtocols assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPSecPermitIPProtocols(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPSecPermitIPProtocols))
        {
            value = m_IPSecPermitIPProtocols;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPSecPermitTCPPorts assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPSecPermitTCPPorts(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPSecPermitTCPPorts))
        {
            value = m_IPSecPermitTCPPorts;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPSecPermitUDPPorts assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPSecPermitUDPPorts(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPSecPermitUDPPorts))
        {
            value = m_IPSecPermitUDPPorts;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPSubnet assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPSubnet(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPSubnet))
        {
            value = m_IPSubnet;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPUseZeroBroadcast assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPUseZeroBroadcast(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPUseZeroBroadcast))
        {
            value = m_IPUseZeroBroadcast;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPXAddress assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPXAddress(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPXAddress))
        {
            value = m_IPXAddress;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPXEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPXEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPXEnabled))
        {
            value = m_IPXEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPXFrameType assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPXFrameType(vector<Uint32> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPXFrameType))
        {
            value = m_IPXFrameType;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPXMediaType assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPXMediaType(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPXMediaType))
        {
            value = m_IPXMediaType;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPXNetworkNumber assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPXNetworkNumber(vector<wstring> &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPXNetworkNumber))
        {
            value = m_IPXNetworkNumber;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! IPXVirtualNetNumber assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetIPXVirtualNetNumber(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eIPXVirtualNetNumber))
        {
            value = m_IPXVirtualNetNumber;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! KeepAliveInterval assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetKeepAliveInterval(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eKeepAliveInterval))
        {
            value = m_KeepAliveInterval;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! KeepAliveTime assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetKeepAliveTime(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eKeepAliveTime))
        {
            value = m_KeepAliveTime;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! MACAddress assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetMACAddress(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eMACAddress))
        {
            value = m_MACAddress;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! MTU assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetMTU(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eMTU))
        {
            value = m_MTU;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! NumForwardPackets assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetNumForwardPackets(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eNumForwardPackets))
        {
            value = m_NumForwardPackets;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! PMTUBHDetectEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetPMTUBHDetectEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(ePMTUBHDetectEnabled))
        {
            value = m_PMTUBHDetectEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! PMTUDiscoveryEnabled assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetPMTUDiscoveryEnabled(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(ePMTUDiscoveryEnabled))
        {
            value = m_PMTUDiscoveryEnabled;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! ServiceName assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetServiceName(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eServiceName))
        {
            value = m_ServiceName;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! SettingID assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetSettingID(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eSettingID))
        {
            value = m_SettingID;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! TcpipNetbiosOptions assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetTcpipNetbiosOptions(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eTcpipNetbiosOptions))
        {
            value = m_TcpipNetbiosOptions;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! TcpMaxConnectRetransmissions assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetTcpMaxConnectRetransmissions(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eTcpMaxConnectRetransmissions))
        {
            value = m_TcpMaxConnectRetransmissions;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! TcpMaxDataRetransmissions assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetTcpMaxDataRetransmissions(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eTcpMaxDataRetransmissions))
        {
            value = m_TcpMaxDataRetransmissions;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! TcpNumConnections assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetTcpNumConnections(Uint32 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eTcpNumConnections))
        {
            value = m_TcpNumConnections;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! TcpUseRFC1122UrgentPointer assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetTcpUseRFC1122UrgentPointer(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eTcpUseRFC1122UrgentPointer))
        {
            value = m_TcpUseRFC1122UrgentPointer;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! TcpWindowSize assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetTcpWindowSize(Uint16 &value) const
    {
        bool supported = false;
        if (IsValueKnown(eTcpWindowSize))
        {
            value = m_TcpWindowSize;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! WINSEnableLMHostsLookup assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetWINSEnableLMHostsLookup(bool &value) const
    {
        bool supported = false;
        if (IsValueKnown(eWINSEnableLMHostsLookup))
        {
            value = m_WINSEnableLMHostsLookup;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! WINSHostLookupFile assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetWINSHostLookupFile(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eWINSHostLookupFile))
        {
            value = m_WINSHostLookupFile;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! WINSPrimaryServer assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetWINSPrimaryServer(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eWINSPrimaryServer))
        {
            value = m_WINSPrimaryServer;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! WINSScopeID assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetWINSScopeID(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eWINSScopeID))
        {
            value = m_WINSScopeID;
            supported = true;
        }
        return supported;
    }
    
    /*----------------------------------------------------------------------------*/
    //! WINSSecondaryServer assigned to interface
    //! param[out] value    Reference to value of property to be initialized
    //! returns    true iff property is supported
    bool NetworkInterfaceConfigurationInstance::GetWINSSecondaryServer(wstring &value) const
    {
        bool supported = false;
        if (IsValueKnown(eWINSSecondaryServer))
        {
            value = m_WINSSecondaryServer;
            supported = true;
        }
        return supported;
    }
}
