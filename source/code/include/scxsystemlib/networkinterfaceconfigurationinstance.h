/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        networkinterfaceconfigurationinstance.h

    \brief       Specification of network interface configuration instance PAL 
    
    \date        12-30-12 11:19:02
    
*/
#ifndef NETWORKINTERFACECONFIGURATIONINSTANCE_H
#define NETWORKINTERFACECONFIGURATIONINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxtime.h>
#include <scxsystemlib/networkinterfaceinstance.h>
#include <vector>
#include <bitset>

// Including MI.h here produces a compile error
typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{

    class NetworkInterfaceConfigurationInfo;

    /*----------------------------------------------------------------------------*/
    /**
        Represent a network interface
    */
    class NetworkInterfaceConfigurationInstance : public EntityInstance
    {
    public:
        enum OptionalAttribute
        {
            eArpAlwaysSourceRoute,
            eArpUseEtherSNAP,
            eCaption,
            eDatabasePath,
            eDeadGWDetectEnabled,
            eDefaultIPGateway,
            eDefaultTOS,
            eDefaultTTL,
            eDescription,
            eDHCPEnabled,
            eDHCPLeaseExpires,
            eDHCPLeaseObtained,
            eDHCPServer,
            eDNSDomain,
            eDNSDomainSuffixSearchOrder,
            eDNSEnabledForWINSResolution,
            eDNSHostName,
            eDNSServerSearchOrder,
            eDomainDNSRegistrationEnabled,
            eForwardBufferMemory,
            eFullDNSRegistrationEnabled,
            eGatewayCostMetric,
            eIGMPLevel,
            eIndex,
            eInterfaceIndex,
            eIPAddress,
            eIPConnectionMetric,
            eIPEnabled,
            eIPFilterSecurityEnabled,
            eIPPortSecurityEnabled,
            eIPSecPermitIPProtocols,
            eIPSecPermitTCPPorts,
            eIPSecPermitUDPPorts,
            eIPSubnet,
            eIPUseZeroBroadcast,
            eIPXAddress,
            eIPXEnabled,
            eIPXFrameType,
            eIPXMediaType,
            eIPXNetworkNumber,
            eIPXVirtualNetNumber,
            eKeepAliveInterval,
            eKeepAliveTime,
            eMACAddress,
            eMTU,
            eNumForwardPackets,
            ePMTUBHDetectEnabled,
            ePMTUDiscoveryEnabled,
            eServiceName,
            eSettingID,
            eTcpipNetbiosOptions,
            eTcpMaxConnectRetransmissions,
            eTcpMaxDataRetransmissions,
            eTcpNumConnections,
            eTcpUseRFC1122UrgentPointer,
            eTcpWindowSize,
            eWINSEnableLMHostsLookup,
            eWINSHostLookupFile,
            eWINSPrimaryServer,
            eWINSScopeID,
            eWINSSecondaryServer,
            SIZE // This should always be last
        };
        
        friend class NetworkInterfaceConfigurationEnumeration;
        
        /*----------------------------------------------------------------------------*/
        //! Constructor
        NetworkInterfaceConfigurationInstance();   
        
        /*----------------------------------------------------------------------------*/
        //! Constructor
        //! \param[in]   info     Initial information
        NetworkInterfaceConfigurationInstance(const NetworkInterfaceInstance &instance);
        
        /*----------------------------------------------------------------------------*/
        //! Destructor
        ~NetworkInterfaceConfigurationInstance();
        //! Check if the value of an attribute is known
        //! \param[in] attr  Attribbute of interest
        //! \return true if the value of the optional attribute is present
        bool IsValueKnown(OptionalAttribute attr) const { return m_knownAttributesBitset[attr]; }
        void SetKnown(OptionalAttribute attr) { m_knownAttributesBitset[attr] = true; }
        
        /*----------------------------------------------------------------------------*/
        //! Name of interface
        //! \return Name
        wstring GetName() const;
        
        /*----------------------------------------------------------------------------*/
        //! ArpAlwaysSourceRoute assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetArpAlwaysSourceRoute(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! ArpUseEtherSNAP assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetArpUseEtherSNAP(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! Caption assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetCaption(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! DatabasePath assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDatabasePath(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! DeadGWDetectEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDeadGWDetectEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! DefaultIPGateway assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDefaultIPGateway(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! DefaultTOS assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDefaultTOS(Uint8 &value) const;

        /*----------------------------------------------------------------------------*/
        //! DefaultTTL assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDefaultTTL(Uint8 &value) const;

        /*----------------------------------------------------------------------------*/
        //! Description assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDescription(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! DHCPEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDHCPEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! DHCPLeaseExpires assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDHCPLeaseExpires(SCXCalendarTime &value) const;

        /*----------------------------------------------------------------------------*/
        //! DHCPLeaseObtained assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDHCPLeaseObtained(SCXCalendarTime &value) const;

        /*----------------------------------------------------------------------------*/
        //! DHCPServer assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDHCPServer(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! DNSDomain assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDNSDomain(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! DNSDomainSuffixSearchOrder assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDNSDomainSuffixSearchOrder(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! DNSEnabledForWINSResolution assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDNSEnabledForWINSResolution(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! DNSHostName assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDNSHostName(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! DNSServerSearchOrder assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDNSServerSearchOrder(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! DomainDNSRegistrationEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetDomainDNSRegistrationEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! ForwardBufferMemory assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetForwardBufferMemory(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! FullDNSRegistrationEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetFullDNSRegistrationEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! GatewayCostMetric assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetGatewayCostMetric(vector<Uint16> &value) const;

        /*----------------------------------------------------------------------------*/
        //! IGMPLevel assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIGMPLevel(Uint8 &value) const;

        /*----------------------------------------------------------------------------*/
        //! Index assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIndex(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! InterfaceIndex assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetInterfaceIndex(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPAddress assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPAddress(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPConnectionMetric assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPConnectionMetric(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPFilterSecurityEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPFilterSecurityEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPPortSecurityEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPPortSecurityEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPSecPermitIPProtocols assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPSecPermitIPProtocols(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPSecPermitTCPPorts assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPSecPermitTCPPorts(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPSecPermitUDPPorts assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPSecPermitUDPPorts(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPSubnet assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPSubnet(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPUseZeroBroadcast assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPUseZeroBroadcast(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPXAddress assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPXAddress(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPXEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPXEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPXFrameType assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPXFrameType(vector<Uint32> &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPXMediaType assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPXMediaType(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPXNetworkNumber assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPXNetworkNumber(vector<wstring> &value) const;

        /*----------------------------------------------------------------------------*/
        //! IPXVirtualNetNumber assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetIPXVirtualNetNumber(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! KeepAliveInterval assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetKeepAliveInterval(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! KeepAliveTime assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetKeepAliveTime(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! MACAddress assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetMACAddress(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! MTU assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true iff property is supported
        bool GetMTU(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! NumForwardPackets assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetNumForwardPackets(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! PMTUBHDetectEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetPMTUBHDetectEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! PMTUDiscoveryEnabled assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetPMTUDiscoveryEnabled(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! ServiceName assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetServiceName(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! SettingID assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetSettingID(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! TcpipNetbiosOptions assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetTcpipNetbiosOptions(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! TcpMaxConnectRetransmissions assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetTcpMaxConnectRetransmissions(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! TcpMaxDataRetransmissions assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetTcpMaxDataRetransmissions(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! TcpNumConnections assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetTcpNumConnections(Uint32 &value) const;

        /*----------------------------------------------------------------------------*/
        //! TcpUseRFC1122UrgentPointer assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetTcpUseRFC1122UrgentPointer(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! TcpWindowSize assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetTcpWindowSize(Uint16 &value) const;

        /*----------------------------------------------------------------------------*/
        //! WINSEnableLMHostsLookup assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetWINSEnableLMHostsLookup(bool &value) const;

        /*----------------------------------------------------------------------------*/
        //! WINSHostLookupFile assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetWINSHostLookupFile(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! WINSPrimaryServer assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetWINSPrimaryServer(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! WINSScopeID assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetWINSScopeID(wstring &value) const;

        /*----------------------------------------------------------------------------*/
        //! WINSSecondaryServer assigned to interface
        //! param[out] value Reference to value of property to be initialized
        //! \return true if property is supported
        bool GetWINSSecondaryServer(wstring &value) const;

    private:
        bool m_knownAttributesBitset[61];
        SCXCoreLib::SCXLogHandle m_log;  /**< Log handle */
        bool m_ArpAlwaysSourceRoute;
        bool m_ArpUseEtherSNAP;
        wstring m_Caption;
        wstring m_DatabasePath;
        bool m_DeadGWDetectEnabled;
        std::vector<wstring> m_DefaultIPGateway;
        Uint8 m_DefaultTOS;
        Uint8 m_DefaultTTL;
        wstring m_Description;
        bool m_DHCPEnabled;
        SCXCalendarTime m_DHCPLeaseExpires;
        SCXCalendarTime m_DHCPLeaseObtained;
        wstring m_DHCPServer;
        wstring m_DNSDomain;
        std::vector<wstring> m_DNSDomainSuffixSearchOrder;
        bool m_DNSEnabledForWINSResolution;
        wstring m_DNSHostName;
        std::vector<wstring> m_DNSServerSearchOrder;
        bool m_DomainDNSRegistrationEnabled;
        Uint32 m_ForwardBufferMemory;
        bool m_FullDNSRegistrationEnabled;
        std::vector<Uint16> m_GatewayCostMetric;
        Uint8 m_IGMPLevel;
        Uint32 m_Index;
        std::vector<wstring> m_IPAddress;
        Uint32 m_IPConnectionMetric;
        bool m_IPEnabled;
        bool m_IPFilterSecurityEnabled;
        bool m_IPPortSecurityEnabled;
        std::vector<wstring> m_IPSecPermitIPProtocols;
        std::vector<wstring> m_IPSecPermitTCPPorts;
        std::vector<wstring> m_IPSecPermitUDPPorts;
        unsigned m_InterfaceIndex;
        std::vector<wstring> m_IPSubnet;
        bool m_IPUseZeroBroadcast;
        wstring m_IPXAddress;
        bool m_IPXEnabled;
        std::vector<Uint32> m_IPXFrameType;
        Uint32 m_IPXMediaType;
        std::vector<wstring> m_IPXNetworkNumber;
        wstring m_IPXVirtualNetNumber;
        Uint32 m_KeepAliveInterval;
        Uint32 m_KeepAliveTime;
        wstring m_MACAddress;
        Uint32 m_MTU;
        Uint32 m_NumForwardPackets;
        bool m_PMTUBHDetectEnabled;
        bool m_PMTUDiscoveryEnabled;
        wstring m_ServiceName;
        wstring m_SettingID;
        Uint32 m_TcpipNetbiosOptions;
        Uint32 m_TcpMaxConnectRetransmissions;
        Uint32 m_TcpMaxDataRetransmissions;
        Uint32 m_TcpNumConnections;
        bool m_TcpUseRFC1122UrgentPointer;
        Uint16 m_TcpWindowSize;
        bool m_WINSEnableLMHostsLookup;
        wstring m_WINSHostLookupFile;
        wstring m_WINSPrimaryServer;
        wstring m_WINSScopeID;
        wstring m_WINSSecondaryServer;
    };
}

#endif /* NETWORKCONFIGURATIONINTERFACE_H */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
