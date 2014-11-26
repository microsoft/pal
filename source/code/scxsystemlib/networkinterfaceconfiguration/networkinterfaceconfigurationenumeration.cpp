/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        networkinterfaceconfigurationenumeration.h

    \brief       Specification of network interface configuration enumeration PAL 
    
    \date        01-20-12 11:19:02
    
*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxsystemlib/scxgateway.h>
#include <scxsystemlib/scxdhcplease.h>
#include <scxcorelib/stringaid.h>
#include <scxsystemlib/networkinterfaceconfigurationenumeration.h>
#include <scxsystemlib/networkinterface.h>
#include <scxcorelib/scxregex.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxsystemlib/processenumeration.h>
#include <iomanip>

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib {

//! Constructs an enumeration dependent on the actual system
NetworkInterfaceConfigurationEnumeration::NetworkInterfaceConfigurationEnumeration(
        SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps):  
        m_deps(deps) 
{    
}

std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > NetworkInstanceConfigurationEnumerationDeps::Find(const std::wstring &name, ProcessEnumeration &procEnum)
{
    return std::vector<SCXCoreLib::SCXHandle<ProcessInstance> >(procEnum.Find(name));
}

std::vector<NetworkInterfaceConfigurationInstance> NetworkInterfaceConfigurationEnumeration::FindAll()
{
    // Use NetworkInterface provider to get all the interfaces
    std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(m_deps);

    std::vector<NetworkInterfaceConfigurationInstance> resultList;
    for (size_t nr = 0; nr < interfaces.size(); nr++)
    {
        NetworkInterfaceConfigurationInstance instance(interfaces[nr]);

        // Set m_Index
        instance.m_Index = static_cast<Uint32>(nr);
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eIndex);

        // Get if interface is enabled. If up interface has the address set. If running inteface has the resources
        // allocated and is ready to receive/transmit.
        if (interfaces[nr].IsKnownIfUp() && interfaces[nr].IsKnownIfRunning())
        {
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eIPEnabled);
            instance.m_IPEnabled = false;
            if (interfaces[nr].IsUp() && interfaces[nr].IsRunning())
            {
                instance.m_IPEnabled = true;
            }
        }

        // (WI 468917) Activate MAC Address to be reported by the provider.
        interfaces[nr].GetMACAddress(instance.m_MACAddress);
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eMACAddress);

        scxulong mtu;
        if (interfaces[nr].GetMTU(mtu))
        {
            instance.m_MTU = static_cast<Uint32>(mtu);
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eMTU);
        }

        instance.m_IPAddress.clear();
        if (interfaces[nr].IsIPAddressKnown())
        {
            instance.m_IPAddress.push_back(interfaces[nr].GetIPAddress());
        }
        vector<wstring> tmpIPv6 = interfaces[nr].GetIPV6Address();
        instance.m_IPAddress.insert(instance.m_IPAddress.end(), tmpIPv6.begin(), tmpIPv6.end());
        if (instance.m_IPAddress.empty() == false)
        {
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eIPAddress);
        }

        if (interfaces[nr].IsNetmaskKnown())
        {
            instance.m_IPSubnet.resize(1);
            instance.m_IPSubnet[0] = interfaces[nr].GetNetmask();
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eIPSubnet);
        }

        // Determine m_ArpUseEtherSNAP
        // ARP packets can be sent using EtherType fields in Ethernet II (DIX) format or in 802.3 (SNAP) format.
        // Both are permitted.  Some operating systems can force ARP packets to be sent in the newer 802.3
        // format, but due to its simplicity, the older DIX format is still widely used.
#if defined(linux) || defined(sun)
        instance.m_ArpUseEtherSNAP = false;
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eArpUseEtherSNAP);
#endif

        // Determine m_Caption
        // Index in format [nnnnnnnn] followed by a short textual description (one-line string) of the object.
        ostringstream ixstr;
        ixstr << '[' << setfill('0') << setw(8) << nr << "] ";
        instance.m_Caption = StrFromUTF8(ixstr.str()) + instance.GetName();
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eCaption);

        // Determine m_Description
        // Description of the CIM_Setting object. This property is inherited from CIM_Setting.
        instance.m_Description = instance.GetName();
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eDescription);

        // Determine m_DeadGWDetectEnabled
        // If TRUE, dead gateway detection occurs. With this feature enabled, Transmission
        // Control Protocol (TCP) asks Internet Protocol (IP) to change to a backup gateway
        // if it retransmits a segment several times without receiving a response.
        // 
        // For Linux, this capability was added with IPv6 via its Neighbor Discovery protocol
        // Solaris also.
#if defined(linux)
        // Subset of neighbor reachability problems solved by IPv6
        instance.m_DeadGWDetectEnabled = SCXDirectory::Exists(L"/proc/sys/net/ipv6");
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eDeadGWDetectEnabled);
#endif

        // Determine m_DefaultTOS
        // Default Type Of Service (TOS) value set in the header of outgoing IP packets.
        // Request for Comments (RFC) 791 defines the values. Default: 0 (zero),
        // Valid Range: 0 - 255.
        // This is deprecated.  Implementations also are not uniform.
        // NOT SET--------------

        // Determine m_DefaultTTL
        // Default Time To Live (TTL) value set in the header of outgoing IP packets.
        // The TTL specifies the number of routers an IP packet can pass through to
        // reach its destination before being discarded. Each router decrements by
        // one the TTL count of a packet as it passes through and discards the
        // packets if the TTL is 0 (zero). Default: 32, Valid Range: 1 - 255.
        std::vector<wstring> lines;
        SCXStream::NLFs nlfs;
#if defined(PF_DISTRO_REDHAT) || defined(sun)
        // This is hardcoded in Linux microkernel source.  Since V2.2 it has been in net/ipv4/ipconfig.c
        // For Solaris it has been 64 since version 2.8
        instance.m_DefaultTTL = 64;
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eDefaultTTL);
#elif defined(PF_DISTRO_SUSE) || defined(PF_DISTRO_ULINUX)
        // if /proc/sys/net/ipv4/ip_default_ttl exists, then override 64 as the default with what is contained
        SCXFile::ReadAllLines(SCXFilePath(L"/proc/sys/net/ipv4/ip_default_ttl"), lines, nlfs);
        if (lines.size() > 0)
        {
            instance.m_DefaultTTL = (Uint8) StrToUInt(lines[0]);
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eDefaultTTL);
        }
#endif

        // Determine m_DHCPEnabled
        // If TRUE, the dynamic host configuration protocol (DHCP) server automatically
        // assigns an IP address to the computer system when establishing a network connection.
        
        // First fetch config data from the appropriate file
        wstring interface = instance.GetName();

        lines.clear();
        bool performRead = true;
#if defined(PF_DISTRO_SUSE)
        // Determine DHCP Enabled by looking for process dhcpcd w/param matching this interface name
        instance.m_DHCPEnabled = GetDHCPEnabledFromProcessList(interface);
#elif defined(PF_DISTRO_REDHAT)
        SCXFile::ReadAllLines(SCXFilePath(SCXCoreLib::StrAppend(L"/etc/sysconfig/network-scripts/ifcfg-", interface)), lines, nlfs);
#elif defined(PF_DISTRO_ULINUX)
        instance.m_DHCPEnabled = GetDHCPEnabledFromProcessList(interface);
        if (!instance.m_DHCPEnabled)
        {
            if (SCXFile::Exists(SCXCoreLib::StrAppend(L"/etc/sysconfig/network-scripts/ifcfg-", interface)))
            {
                SCXFile::ReadAllLines(SCXFilePath(SCXCoreLib::StrAppend(L"/etc/sysconfig/network-scripts/ifcfg-", interface)), lines, nlfs);
            }
            else if (SCXFile::Exists(SCXCoreLib::StrAppend(L"/etc/sysconfig/network/ifcfg-", interface)))
            {
                SCXFile::ReadAllLines(SCXFilePath(SCXCoreLib::StrAppend(L"/etc/sysconfig/network/ifcfg-", interface)), lines, nlfs);
            }
            else
            {
                performRead = false;
            }
        }
#elif defined(sun)
        SCXFile::ReadAllLines(SCXFilePath(SCXCoreLib::StrAppend(L"/etc/hostname.", interface)), lines, nlfs);
#elif defined(hpux)
        SCXFile::ReadAllLines(SCXFilePath(L"/etc/rc.config.d/netconf"), lines, nlfs);
#elif defined(aix)
        SCXFile::ReadAllLines(SCXFilePath(L"/etc/dhcpcd.ini"), lines, nlfs);
#endif

        if (performRead)
        {
            instance.m_DHCPEnabled = GetDHCPEnabledFromConfigData(lines, interface);
        }

#if defined(linux)
        // Determine DHCP Enabled by looking for process dhcpcd w/param matching this interface name
        if (!instance.m_DHCPEnabled)
        {
            instance.m_DHCPEnabled = GetDHCPEnabledFromProcessList(interface);
        }
#endif

        instance.SetKnown(NetworkInterfaceConfigurationInstance::eDHCPEnabled);
        
#if defined(linux) || defined(sun) || defined(hpux)
        // Initialize the DHCP lease information
        DHCPLeaseInfo dhcpLeaseInfo(interface);
        
        // Determine m_DHCPLeaseExpires
        // Expiration date and time for a leased IP address that was assigned to the
        // computer by the dynamic host configuration protocol (DHCP) server.
        // Example: 20521201000230.000000000
        instance.m_DHCPLeaseExpires = dhcpLeaseInfo.getLeaseExpires();
        if (instance.m_DHCPLeaseExpires.IsInitialized())
        {
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eDHCPLeaseExpires);
        }
        
        // Determine m_DHCPLeaseObtained
        // Date and time the lease was obtained for the IP address assigned to the
        // computer by the dynamic host configuration protocol (DHCP) server.
        // Example: 19521201000230.000000000
        instance.m_DHCPLeaseObtained = dhcpLeaseInfo.getLeaseObtained();
        if (instance.m_DHCPLeaseObtained.IsInitialized())
        {
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eDHCPLeaseObtained);
        }
#endif

        // Determine m_DHCPServer
        // IP address of the dynamic host configuration protocol (DHCP) server.
        // Example: 10.55.34.2
#if defined(linux)
        instance.m_DHCPServer = dhcpLeaseInfo.getDHCPServer();
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eDHCPServer);
#endif

        // Determine m_DefaultIPGateway
        // Array of IP addresses of default gateways that the computer system uses.
        // Example: 192.168.12.1 192.168.46.1
#if defined(linux) || defined(sun) || defined(aix)
        wstring gwip;
        if (GatewayInfo::get_gatewayip(gwip, m_deps))
        {
            instance.m_DefaultIPGateway.push_back(gwip);
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eDefaultIPGateway);
        }
#elif defined(hpux)
        // The HPUX DHCP info file also contains the default gateway
        wstring defaultGateway = dhcpLeaseInfo.getDefaultGateway();
        if (defaultGateway.size() > 0)
        {
            instance.m_DefaultIPGateway.push_back(defaultGateway);
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eDefaultIPGateway);
        }
#endif

        // Determine m_DNSDomain
        // Organization name followed by a period and an extension that indicates
        // the type of organization, such as microsoft.com. The name can be any
        // combination of the letters A through Z, the numerals 0 through 9, and
        // the hyphen (-), plus the period (.) character used as a separator.
        // Example: microsoft.com
#if defined(linux) || defined(sun) || defined(hpux)
        instance.m_DNSDomain = dhcpLeaseInfo.getDomainName();
        if (instance.m_DNSDomain.empty() == false)
        {
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eDNSDomain);
        }
#endif

        // Determine m_DNSDomainSuffixSearchOrder
        // Array of DNS domain suffixes to be appended to the end of host names
        // during name resolution. When attempting to resolve a fully qualified
        // domain name (FQDN) from a host-only name, the system will first append
        // the local domain name. If this is not successful, the system will use
        // the domain suffix list to create additional FQDNs in the order listed
        // and query DNS servers for each.
        // Example: samples.microsoft.com example.microsoft.com
        // Linux doesn't add missing suffixes -----NOT SET

        // Determine m_DNSEnabledForWINSResolution
        // If TRUE, the Domain Name System (DNS) is enabled for name resolution
        // over Windows Internet Naming Service (WINS) resolution. If the name
        // cannot be resolved using DNS, the name request is forwarded to WINS
        // for resolution.
        // Windows only --------NOT SET

        // Determine m_DNSHostName
        // Host name used to identify the local computer for authentication by
        // some utilities. Other TCP/IP-based utilities can use this value to
        // acquire the name of the local computer. Host names are stored on DNS
        // servers in a table that maps names to IP addresses for use by DNS.
        // The name can be any combination of the letters A through Z, the
        // numerals 0 through 9, and the hyphen (-), plus the period (.) character
        // used as a separator. By default, this value is the Microsoft networking
        // computer name, but the network administrator can assign another host
        // name without affecting the computer name.
        // Example: corpdns
        // Probably utility authentication for Windows only ----------NOT SET

        // Determine m_DNSServerSearchOrder
        // Array of server IP addresses to be used in querying for DNS servers.
#if defined(linux) || defined(sun) || defined(hpux)
        lines.clear();
        instance.m_DNSServerSearchOrder.clear();
        SCXFile::ReadAllLines(SCXFilePath(L"/etc/resolv.conf"), lines, nlfs);
        for (uint i = 0; i < lines.size(); i++)
        {
            // remove all comments from the current line, as they should be ignored
            wstring curLine = lines[i];
            wstring::size_type pos;
            pos = curLine.find(L";");
            if (pos != wstring::npos)
            {
                curLine.erase(pos, curLine.length());
            }
            if (curLine.empty())
            {
                continue;
            }
            
            std::vector<wstring> tokens;
            StrTokenize(curLine, tokens, L" \t");
            if (tokens.size() > 1 && tokens[0].compare(L"nameserver") == 0)
            {
                instance.m_DNSServerSearchOrder.push_back(tokens[1]);
            }
        }
        if(instance.m_DNSServerSearchOrder.size() > 0)
        {
            instance.SetKnown(NetworkInterfaceConfigurationInstance::eDNSServerSearchOrder);
        }
#endif

        // Determine m_DomainDNSRegistrationEnabled
        // If TRUE, the IP addresses for this connection are registered in DNS under the domain
        // name of this connection in addition to being registered under the computer's
        // full DNS name. The domain name of this connection is either set using the
        // SetDNSDomain() method or assigned by DSCP. The registered name is the host
        // name of the computer with the domain name appended.
        // Windows 2000:  This property is not available.
        // NOT SET for Linux------------------------------------------------------
        
        // Determine m_ArpAlwaysSourceRoute
        // If TRUE, TCP/IP transmits Address Resolution Protocol (ARP) queries with
        // source routing enabled on Token Ring networks. By default (FALSE), ARP first
        // queries without source routing, and then retries with source routing enabled
        // if no reply is received. Source routing allows the routing of network packets
        // across different types of networks.
        // See http://www.rapid7.com/vulndb/lookup/generic-ip-source-routing-enabled for AIX/Linux/Solaris

        // This property seems to be set to prevent ARP spoofing.  The
        // principle of ARP spoofing is to send fake ARP messages onto a LAN.
        // Generally, the aim is to associate the attacker's MAC address with
        // the IP address of another host (such as the default gateway).
        // 
        // Operating System Approaches
        // Static ARP entries: entries in the local ARP cache can be defined as
        // static to prevent overwrite. While static entries provide perfect security
        // against spoofing if the operating systems handles them correctly, they
        // result in quadratic maintenance efforts as IP-MAC mappings of all machines
        // in the network have to be distributed to all other machines.
        // OS security: Operating systems react differently, e.g. Linux ignores
        // unsolicited replies, but on the other hand uses seen requests from
        // other machines to update its cache. Solaris only accepts updates on
        // entries after a timeout. In Microsoft Windows, the behavior of the
        // ARP cache can be configured through several registry entries under
        // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters,
        // namely ArpCacheLife, ArpCacheMinReferenceLife, ArpUseEtherSNAP, ArpTRSingleRoute,
        // ArpAlwaysSourceRoute, ArpRetryCount.
#if defined(linux)
        instance.m_ArpAlwaysSourceRoute = false;
        lines.clear();
        SCXFile::ReadAllLines(SCXFilePath(L"/etc/sysctl.conf"), lines, nlfs);
        SCXRegex r(L"accept_source_route\\s*=\\s*([01])");

        instance.m_ArpAlwaysSourceRoute = false;
        for (int i = 0; i < (int)lines.size(); i++)
        {
            if (r.IsMatch(lines[i]))
            {
                instance.m_ArpAlwaysSourceRoute = true;
                break;
            }
        }
        instance.SetKnown(NetworkInterfaceConfigurationInstance::eArpAlwaysSourceRoute);
#endif
        resultList.push_back(instance);
    }
    return resultList;
}

    /*----------------------------------------------------------------------------*/
    //! Destructor
    NetworkInterfaceConfigurationEnumeration::~NetworkInterfaceConfigurationEnumeration()
    {
    }
    
    /*----------------------------------------------------------------------------*/
    //! Implementation of the Init method of the entity framework.
    void NetworkInterfaceConfigurationEnumeration::Init()
    {
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
    void NetworkInterfaceConfigurationEnumeration::Update(bool updateInstances)
    {
        if (updateInstances)
        {
            UpdateInstances();        
        }
        else
        {
            UpdateEnumeration();
        }
        
    }
    
    /*----------------------------------------------------------------------------*/
    //! Run the Update() method on all instances in the colletion, including the
    //! Total instance if any.
    //! \note Optimized implementation that recreates the same result as running update on
    //!       each instance, but does not actually do so
    void NetworkInterfaceConfigurationEnumeration::UpdateInstances()
    {
        // For each NetworkInterface in the subclass,
        //   find the corresponding entry in NetworkInterfaceInfo::FindAll(m_deps)
        //   call the entry's Update method passing the corresponding entry as arg
    }
    
    /*----------------------------------------------------------------------------*/
    //! Make the enumeration correspond to the current state of the system
    void NetworkInterfaceConfigurationEnumeration::UpdateEnumeration()
    {
    }
    
    /*----------------------------------------------------------------------------*/
    /**
        Get DHCPEnabled status from the process list
    */
    bool NetworkInterfaceConfigurationEnumeration::GetDHCPEnabledFromProcessList(
            std::wstring& name,
            SCXCoreLib::SCXHandle<NetworkInstanceConfigurationEnumerationDeps> deps)
    {
        ProcessEnumeration procEnum;
        procEnum.SampleData();
        procEnum.Update(true);

        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > proc_v = deps->Find(L"dhcpcd", procEnum); 
        if(proc_v.size() == 0)
          return false; 
        
        for(std::vector<SCXCoreLib::SCXHandle<ProcessInstance> >::iterator i = proc_v.begin(); i != proc_v.end(); ++i)
        {
             SCXCoreLib::SCXHandle<ProcessInstance> process = (*i);
             std::vector<std::string> parms;  
             if(!process->GetParameters(parms))
                  continue; 
            
             for(std::vector<std::string>::iterator i_parms = parms.begin(); i_parms != parms.end(); i_parms++)
               if(name.compare(SCXCoreLib::StrFromUTF8(*i_parms)) == 0)
                 return true;
        } 
        
        return false;
    }
    
    /*----------------------------------------------------------------------------*/
    /**
        Get DHCPEnabled status from the platform's config file
        \param configData - contents of the DHCP config file
        \param interface - the interface for which we want DHCPEnabled status
    */
    bool NetworkInterfaceConfigurationEnumeration::GetDHCPEnabledFromConfigData
    (
        std::vector<wstring>configData,
        wstring interface
    )
    {
        if (interface.size() + configData.size() == 0)
        {
            return false;
        }
#if defined(PF_DISTRO_REDHAT) || defined(PF_DISTRO_ULINUX) || defined(PF_DISTRO_SUSE)
        /* Typical file
        DEVICE="eth0" # This is also in the file name
        BOOTPROTO="dhcp"
        HWADDR="00:21:5E:DB:AC:98"
        ONBOOT="yes"
        */
        bool result = false;

        SCXRegex bootproto(L"BOOTPROTO.*dhcp");
        for (uint i = 0; i < configData.size(); i++)
        {
            wstring line = configData[i];
            if (bootproto.IsMatch(configData[i]))
            {
                result = true;
            }
        }
        return result; 

#elif defined(sun)
        return configData.size() > 0; // If the file exists and has data that means DHCP is enabled
#elif defined(hpux)
        // We are looking for a line
        //
        // DHCP_ENABLED[<i>]="<1|0>"
        //
        // where 1 means enabled and <i> corresponds to a line of the form
        //
        // INTERFACE_NAME[<i>]="<interface name>"
        //
        // and <interface name> matches the member variable of NetworkInterfaceConfigurationInstance instance
        //
        // Example file where we are looking for network interface "lan0"
        /*
        INTERFACE_NAME[0]=lan0     #<---------- for interface "lan0" this means we are looking for index 0
        IP_ADDRESS[0]=10.217.5.127
        SUBNET_MASK[0]=255.255.254.0
        BROADCAST_ADDRESS[0]=""
        INTERFACE_STATE[0]=""
        DHCP_ENABLE[0]=1          #<----------- this is the DHCP_ENABLED line we are therefore interested in
        INTERFACE_MODULES[0]=""
        INTERFACE_NAME[1]=lan666
        IP_ADDRESS[1]=55.5.12.12
        DHCP_ENABLE[1]=0
        */

        bool result = false;

        uint idx = numeric_limits<unsigned int>::max();
        vector<wstring> tokens;
        for (uint i = 0; i < configData.size(); i++)
        {
            wstring line = configData[i];
            StrTokenize(line, tokens, L"[]=");
            if (tokens.size() < 1)
            {
                continue;
            }
            if (tokens[0].compare(L"INTERFACE_NAME") == 0)
            {
                if (tokens[2].compare(interface) == 0)
                {
                  idx = StrToUInt(tokens[1]);
                }
            }
            else if (line.find(L"DHCP_ENABLE") == 0)
            {
                if (idx == StrToUInt(tokens[1]));
                {
                  result = (tokens[2].compare(L"1") == 0) ? 1 : 0;
                }
            }            
        }
        return result; 

#elif defined(aix)
        // The file has entries that look like this: 
        // 
        // # Blah
        // Mumble ... 
        // interface en1
        // {
        //  option 13 0 
        //  option 54 10.152.203.45
        //  option 9 0
        // }
        // We want the interface name to match our interface
        // Then search for option 54, which is 'Server Identifier' according 
        // to 'DHCP Server Configuration File' section of 'AIX 6.1 Information' doc
        // from IBM. 
        // The number at the end of that line is address of the server, or '0' (or blank) 
        // if DHCP is not enabled on this interface.

        bool result = false;
        vector<wstring> tokens;
        for(uint i = 0; i < configData.size(); ++i)
          {

            wstring line = configData[i]; 
            StrTokenize(line, tokens, L"\t "); 
            if(tokens.size() < 2)
              {
                continue; 
              }
            // look for "interface" tag followed by our interface, e.g. "interface en1"
            if(tokens[0].compare(L"interface") == 0 && tokens[1].compare(interface) == 0)
              {
                // we could get cute with recursion here but let's just look for 
                // our line .. if we hit a closing curly brace or "interface" tag
                // call it quits
                
                // Loop starting at the same position in the vector ..
                for(/* i does not change */ ; i < configData.size() ; ++i)
                  {
                    line = configData[i]; 
                    StrTokenize(line, tokens, L"\t "); 
                    if(tokens.size() < 1)
                      continue; 
                    
                    // if this hits we ran past the end of this interface section ..
                    // since we are searching in the interface specified by client, we are done ... 
                    if(tokens[0].compare(L"}") == 0)
                      return false;
                    
                    // The only interesting line looks like "option 54 <IPAddress | 0>" so must have 3 tokens
                    if(tokens.size() < 2)
                      continue;

                    // Get line with an address, look at it .. 
                    if(tokens[0].compare(L"option") == 0 && tokens[1].compare(L"54") == 0)
                      {
                        // no ip address, no DHCP ....
                        if(tokens[2].compare(L"0") == 0)
                          break;  // back to searching for "interface" block 
                        
                        // If address has even one ['.' | ':'] char we will assume it is a good [IP | IPv6] address and return true
                        if(tokens[2].find(L'.') != wstring::npos || tokens[2].find(L':') != wstring::npos)
                          return true;
                        // There can be no two entries of the form "interface <our_interface>" in a file
                        // so no good IP address here means we are done ... 
                        else
                          return false;
                        
                      }
                    
                  }
                
              }
          }
        return result;
#else
        throw SCXNotSupportedException(L"GetDHCPEnabledFromConfigData", SCXSRCLOCATION); 
#endif

    }
}


