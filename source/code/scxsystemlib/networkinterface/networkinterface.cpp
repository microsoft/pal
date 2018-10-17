/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implementation of network interface PAL

    \date        08-03-03 12:12:02

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/networkinterface.h>
#include <scxcorelib/scxassert.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/logsuppressor.h>
#include <scxcorelib/strerror.h>

#include <sstream>
#include <iomanip>

#if defined(hpux)
#include <scxdlpi.h>
#include <stropts.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/dlpi.h>
#include <sys/dlpi_ext.h>
#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stream.h>

#endif
#if defined(aix)
#include <libperfstat.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/kinfo.h>
#include <sys/ndd_var.h> // For nddictl etc...
#include <sys/ndd.h> // For ndd_t structure.
#include <netinet/if_ether.h> // For ETHERMTU
#include <sys/cdli_entuser.h> // For kent_all_stats_t
#include <sys/cdli_entuser.gxent.h> // For gxent_all_stats_t
#include <sys/cdli_entuser.goent.h> // For goent_all_stats_t
#include <sys/cdli_entuser.scent.h> // For scent_all_stats_t
#include <sys/cdli_entuser.phxent.h> // For phxent_all_stats_t Must be included after goent.h
#include <sys/cdli_entuser.ment.h> // For ment_all_stats_t
#include <sys/cdli_entuser.hea.h> // For hea_all_stats_t
#include <sys/cdli_entuser.lncent.h> // For lncent_all_stats_t
#include <sys/cdli_entuser.shient.h> // For shient_all_stats_t
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stropts.h>

//
// Function getkerninfo() has not been documented, but used in IBM examples.
// It also doesn't have prototype in headers, so we declare it here.
//
extern "C" int getkerninfo(int, char *, int *, int32long64_t);
#endif

#include <sys/types.h>
#include <sys/socket.h>
#if !defined(linux)
/* <net/if.h> conflits with <linux/if.h> */
#include <net/if.h>
#endif
#if defined(linux)
#include <linux/if_arp.h>
#include <linux/sockios.h>
#include <linux/types.h>
/* some platforms do not define u32 u16 u8, but <linux/ethtool.h> use these
   types, so define here for compatible */
#if !defined(u64)
#define u64 __u64
#endif

#if !defined(u32)
#define u32 __u32
#endif

#if !defined(u16)
#define u16 __u16
#endif

#if !defined(u8)
#define u8 __u8
#endif
#include <linux/ethtool.h>
#endif

#if defined(sun)
#include <net/if_arp.h>
#elif defined(hpux)
#include <scxcorelib/scxprocess.h>
#endif

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <vector>

#if defined(sun)
#include <sys/sockio.h>
#endif

#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>

using namespace SCXCoreLib;
using namespace std;

namespace  
{
using namespace SCXSystemLib;
using namespace SCXCoreLib;

#define MAC_ADDRESS_BUFFER_SIZE 18  // the size of a MAC address string: xx:xx:xx:xx:xx:xx\0

#if defined(sun) || defined(linux)
    /*----------------------------------------------------------------------------*/
    //! Read a network interface name from a stream
    //! \param[in]  source  To read from
    //! \returns    Network interface name
    wstring ReadInterfaceName(wistream &source) {
        wstring text;
        while (SCXStream::IsGood(source) && source.peek() == ' ') {
            source.get();
        }
        while (SCXStream::IsGood(source) && source.peek() != ':') {
            text += static_cast<wchar_t>(source.get());
        }
        source.get();
        return text;
    }
#endif


    /*----------------------------------------------------------------------------*/
    // RAII encapsulation of file descriptor
    class FileDescriptor {
    public:
        //! Constructor
        //! \param[in]    Descriptor  To be managed
        FileDescriptor(int descriptor) : m_descriptor(descriptor) {
        }

        //! Destructor
        ~FileDescriptor() {
            if (m_descriptor >= 0) {
                close(m_descriptor);
            }
        }

        //! Enable normal use of the descriptor
        operator int() {
            return m_descriptor;
        }

    private:
        int m_descriptor;   //!< Native descriptor to be managed
    };

    /*----------------------------------------------------------------------------*/
    //! Convert a socket address to textual format
    //! \param[in]  addr To be converted
    //! \returns    Textual format
    wstring ToString(sockaddr &addr) {
        std::wostringstream ipAddress;
        ipAddress << (unsigned char) addr.sa_data[2];
        ipAddress << L"." << (unsigned char)addr.sa_data[3];
        ipAddress << L"." << (unsigned char)addr.sa_data[4];
        ipAddress << L"." << (unsigned char)addr.sa_data[5];
        return ipAddress.str();
    }

    //! Convert a MAC address to a string
    //! \returns A string formatted as a MAC address - "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x"
    wstring FormatMacAddress(unsigned int e1,
                               unsigned int e2,
                               unsigned int e3,
                               unsigned int e4,
                               unsigned int e5,
                               unsigned int e6)
    {
        wstringstream addrFormat;

        addrFormat << std::hex << std::setw(2) << std::setfill(L'0') << e1
             << std::hex << std::setw(2) << std::setfill(L'0') << e2
             << std::hex << std::setw(2) << std::setfill(L'0') << e3
             << std::hex << std::setw(2) << std::setfill(L'0') << e4
             << std::hex << std::setw(2) << std::setfill(L'0') << e5
             << std::hex << std::setw(2) << std::setfill(L'0') << e6;

        return addrFormat.str();
    }

    wstring wstrerror(int err)
    {
        return SCXCoreLib::StrFromUTF8(SCXCoreLib::strerror(err));
    }

#if defined(sun)

/*----------------------------------------------------------------------------*/
//! Retrieve the value of an attribute
//! \param[in]    hasAttr     true if the attribute is present
//! \param[in]    attr     value of the attribute
//! \param[out]   attrId   Identifier of attribute
//! \param[out]   knownAttributesMask  Will contain id of attribute if value is present
//! \returns      Value of attribute if present, undefined otherwise
scxulong ValueOf(bool hasAttr, const scxulong attr, NetworkInterfaceInfo::OptionalAttribute attrId, unsigned long &knownAttributesMask) {
    scxulong value = 0;
    if (hasAttr) {
        value = attr;
        knownAttributesMask |= attrId;
    }
    return value;
}

/*----------------------------------------------------------------------------*/
//! Retrieve the "best" value of an attribute
//! \param[in]    hasAttr64   true if the attr64 attribute is present
//! \param[in]    attr64        64-bit attribute value
//! \param[in]    hasAttr       true if the attr attribute is present
//! \param[in]    attr            32-bit attribute value
//! \param[out]   attrId   Identifier of attribute
//! \param[out]   knownAttributesMask  Will contain id of attribute if value is present
//! \returns      Best value of attribute if present, undefined otherwise
scxulong BestValueOf(bool hasAttr64,
                     const scxulong attr64,
                     bool hasAttr, const scxulong attr,
                     NetworkInterfaceInfo::OptionalAttribute attrId,
                     unsigned long &knownAttributesMask) {
    scxulong value = 0;
    if (hasAttr64) {
        value = attr64;
        knownAttributesMask |= attrId;
    } else if (hasAttr) {
        value = attr;
        knownAttributesMask |= attrId;
    }
    return value;
}

#endif
}

namespace SCXSystemLib {

#if defined(aix)
   /*----------------------------------------------------------------------------*/
   //! Set up a map of the network types from NDD to NetworkAdapterTypeIDType
   //!  These pairs can be searched with the results from getkerninfo
   //!  to get the corresponding name we want.
   const SCXSystemLib::NetworkInterfaceInfo::NDDMAPPair SCXSystemLib::NetworkInterfaceInfo::m_netwTypePairs[] =
   {
        NDDMAPPair(NDD_ETHER, SCXSystemLib::eNetworkAdapterTypeEthernetUsingDIXHeaderFormat),
        NDDMAPPair(NDD_ISO88023, SCXSystemLib::eNetworkAdapterTypeEthernet8023),
        NDDMAPPair(NDD_ISO88025, SCXSystemLib::eNetworkAdapterTypeTokenRing8025),
        NDDMAPPair(NDD_FDDI, SCXSystemLib::eNetworkAdapterTypeFDDI),
        NDDMAPPair(NDD_ATM, SCXSystemLib::eNetworkAdapterTypeATM)
   };

   /*----------------------------------------------------------------------------*/
   //! Declare an instance of the Lookup Table itself
   const NetworkInterfaceInfo::NDDMAP
       NetworkInterfaceInfo::m_mapNWAdaptTypeIdToNDDType(m_netwTypePairs,
                                 m_netwTypePairs + (sizeof(m_netwTypePairs)/sizeof(m_netwTypePairs[0])));
#endif


#if defined(sun)

static const wchar_t* KSTAT_MII = L"mii"; //<!kstat name "mii" for net
static const char* KSTAT_CAP_AUTONEG = "cap_autoneg"; //<!//kstat data field name for autosense

#endif
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve availability and status of the device.

        \param       value - output parameter where the availability of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetAvailability(unsigned short& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        if (m_availability >= eAvailabilityOther && m_availability < eAvailabilityCnt)
        {
            value = m_availability;
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"Availability", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve network medium in use of the device.

        \param       value - output parameter where the AdapterType of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetAdapterType(wstring& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(aix) || defined(hpux)
        if (eNetworkAdapterTypeInvalid != m_adapterTypeID)
        {
            value = m_adapterType;
            fRet = true;
        }
#elif defined(sun) || defined(hpux)
        fRet = false;
#else
        throw SCXNotSupportedException(L"AdapterType", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve network medium ID in use of the device.

        \param       value - output parameter where the AdapterTypeID of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetAdapterTypeID(unsigned short& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(aix) || defined(hpux)
        if (eNetworkAdapterTypeInvalid != m_adapterTypeID)
        {
            value = m_adapterTypeID;
            fRet = true;
        }
#elif defined(sun)
        fRet = false;
#else
        throw SCXNotSupportedException(L"AdapterTypeID", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve if the network adapter can automatically determine the speed of the attached or network media.

        \param       value - output parameter where the AutoSense of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetAutoSense(bool& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        if (IsValueKnown(eAutoSense))
        {
            value = m_autoSense;
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"AutoSense", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve index value that uniquely identifies the local network interface of the device.

        \param       value - output parameter where the InterfaceIndex of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetInterfaceIndex(unsigned int& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        if (IsValueKnown(eInterfaceIndex))
        {
            value = m_interfaceIndex;
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"InterfaceIndex", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve media access control address for this network adapter of the device.

        \param       value - output parameter where the MACAddress of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetMACAddressRAW(std::wstring& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        if (!m_macAddress.empty())
        {
            value = m_macAddress;
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"MACAddress", SCXSRCLOCATION);
#endif
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Retrieve the raw form of the media access control address for this network 
      adapter of the device where the case is preserved and there is no delimiter.

      \param value - output parameter where the MACAddress of the device is stored.
      \param sepChar - the delimiter.
      \param upperCase - true for uppercase and false to preserve the case.
      \returns true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetMACAddress(std::wstring& value, 
                                             const char sepChar/* =':' */,
                                             bool upperCase/* =false */) const
    {
        bool fRet = false;

#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        if (!m_macAddress.empty())
        {
            std::wstringstream strOut;
            size_t lenMAC = m_macAddress.length();
            for(size_t pos = 0; pos < lenMAC; pos += 2)
            {
                if (upperCase) 
                {
                    strOut << SCXCoreLib::StrToUpper(m_macAddress.substr(pos, 2));
                }
                else
                { // Don't covert the address to uppercase.
                    strOut << m_macAddress.substr(pos, 2);
                }
                if(pos < lenMAC - 2)
                {
                    // Inject delimiter only if requested.
                    if (sepChar != '\0')
                    {
                        strOut << sepChar;
                    }
                }
            }
            value = strOut.str();
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"MACAddress", SCXSRCLOCATION);
#endif
 
        return fRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve maximum speed, in bits per second, for the network adapter.

        \param       value - output parameter where the MaxSpeed of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetMaxSpeed(scxulong& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        if (0 != m_maxSpeed)
        {
            value = m_maxSpeed;
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"MaxSpeed", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve state of the network adapter connection to the network.

        \param       value - output parameter where the NetConnectionStatus of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetNetConnectionStatus(unsigned short& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        value=m_netConnectionStatus;
        fRet = true;
#else
        throw SCXNotSupportedException(L"NetConnectionStatus", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve whether the adapter is a physical or a logical adapter.

        \param       value - output parameter where the PhysicalAdapter of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetPhysicalAdapter(bool& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(aix) || defined(hpux)
        if (IsValueKnown(ePhysicalAdapter))
        {
            value = m_physicalAdapter;
            fRet = true;
        }
#elif defined(sun)
        fRet = false;
#else
        throw SCXNotSupportedException(L"PhysicalAdapter", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve estimate of the current bandwidth in bits per second.

        \param       value - output parameter where the Speed of the device is stored.
        \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetSpeed(scxulong& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        if (0 != m_speed)
        {
            value = m_speed;
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"Speed", SCXSRCLOCATION);
#endif
        return fRet;
    }
    /*----------------------------------------------------------------------------*/
    /**
       Retrieve the maximum transmission unit (MTU).

       \param       value - output parameter where the MTU of the device is stored.
       \returns     true if value is supported on platform, false otherwise.
    */
    bool NetworkInterfaceInfo::GetMTU(scxulong& value) const
    {
        bool fRet = false;
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        if (IsValueKnown(eMTU))
        {
            value = m_mtu;
            fRet = true;
        }
#else
        throw SCXNotSupportedException(L"MTU", SCXSRCLOCATION);
#endif
        return fRet;
    }
std::vector<wstring> NetworkInterfaceInfo::s_validInterfaces;

    /*----------------------------------------------------------------------------*/
    //! Dump object as string (for logging).
    //! \returns   String representation of object.
    const wstring NetworkInterfaceInfo::DumpString() const
    {
        return SCXDumpStringBuilder("NetworkInterfaceInfo")
            .Text("name", m_name)
            .Scalar("knownAttributesMask", m_knownAttributesMask)
            .Text("ipAddress", m_ipAddress)
            .Text("netmask", m_netmask)
            .Text("broadcastAddress", m_broadcastAddress)
            .Scalar("bytesSent", m_bytesSent)
            .Scalar("bytesReceived", m_bytesReceived)
            .Scalar("packetsSent", m_packetsSent)
            .Scalar("packetsReceived", m_packetsReceived)
            .Scalar("errorsSending", m_errorsSending)
            .Scalar("errorsReceiving", m_errorsReceiving)
            .Scalar("collisions", m_collisions)
            .Scalar("up", m_up)
            .Scalar("running", m_running);
    }


#if defined(sun)

/*----------------------------------------------------------------------------*/
//! Find all network interfaces using the KStat API
//! \param[out]    interfaces       To be populated
//! \param[in]     deps             Dependencies to rely on
//! \throws        SCXErrnoException   KStat problems
void NetworkInterfaceInfo::FindAllUsingKStat(std::vector<NetworkInterfaceInfo> &interfaces,
                                             SCXHandle<NetworkInterfaceDependencies> deps) {

    SCXCoreLib::SCXLogHandle m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.networkinterface"));
    SCX_LOGHYSTERICAL(m_log, L"NetworkInterfaceInfo::FindAllUsingKStat entry");

    SCXCoreLib::SCXHandle<SCXKstat> kstat = deps->CreateKstat();

    for (kstat_t* cur = kstat->ResetInternalIterator(); cur; cur = kstat->AdvanceInternalIterator())
    {
        if (strcmp(cur->ks_class, "net") == 0 && cur->ks_type == KSTAT_TYPE_NAMED )
        {
            scxulong ipackets;
            scxulong opackets;
            scxulong ipackets64;
            scxulong opackets64;
            scxulong rbytes;
            scxulong obytes;
            scxulong rbytes64;
            scxulong obytes64;
            scxulong ierrors;
            scxulong oerrors;
            scxulong collisions;
            scxulong lbufs;
            scxulong ifspeed;

            if ( SCXCoreLib::eHysterical >= m_log.GetSeverityThreshold() )
            {
                std::wstringstream errMsg;
                errMsg << L"FindAllUsingKStat: considering " << StrFromUTF8(cur->ks_name)
                       << L", class: " << StrFromUTF8(cur->ks_class);
                SCX_LOGHYSTERICAL(m_log, errMsg.str());
            }

            // Skip the loopback interface (WI 463810)
            FileDescriptor fd = socket(AF_INET, SOCK_DGRAM, 0);
            struct ifreq ifr;
            memset(&ifr, 0, sizeof(ifr));
            strcpy(ifr.ifr_name, cur->ks_name);

            // if not found with ioctl, don't add an instance and continue the loop
            if (deps->ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0)
            {
                if (ifr.ifr_ifru.ifru_flags & IFF_LOOPBACK)
                    continue;

                bool hasIpackets = kstat->TryGetValue(L"ipackets", ipackets);
                bool hasOpackets = kstat->TryGetValue(L"opackets", opackets);
                bool hasIpackets64 = kstat->TryGetValue(L"ipackets64", ipackets64);
                bool hasOpackets64 = kstat->TryGetValue(L"opackets64", opackets64);
                bool hasRbytes = kstat->TryGetValue(L"rbytes", rbytes);
                bool hasObytes = kstat->TryGetValue(L"obytes", obytes);
                bool hasRbytes64 = kstat->TryGetValue(L"rbytes64", rbytes64);
                bool hasObytes64 = kstat->TryGetValue(L"obytes64", obytes64);
                bool hasIerrors = kstat->TryGetValue(L"ierrors", ierrors);
                bool hasOerrors = kstat->TryGetValue(L"oerrors", oerrors);
                bool hasCollisions = kstat->TryGetValue(L"collisions", collisions);
                bool hasLbufs = kstat->TryGetValue(L"lbufs", lbufs);
                bool hasIfspeed= kstat->TryGetValue(L"ifspeed", ifspeed);

                if (!hasLbufs && (hasIpackets || hasOpackets || hasIpackets64 || hasOpackets64 || hasRbytes || hasObytes
                            || hasRbytes64 || hasObytes64 || hasIerrors || hasOerrors || hasCollisions))
                {
                    SCX_LOGHYSTERICAL(m_log, StrAppend(wstring(L"FindAllUsingKStat: Adding instrance "),
                                                       StrFromUTF8(cur->ks_name)));

                    NetworkInterfaceInfo instance(deps);
                    instance.m_name = StrFromUTF8(cur->ks_name);

                    instance.m_packetsSent = BestValueOf(hasOpackets64, opackets64, hasOpackets, opackets, ePacketsSent, instance.m_knownAttributesMask);
                    instance.m_packetsReceived = BestValueOf(hasIpackets64, ipackets64, hasIpackets, ipackets, ePacketsReceived, instance.m_knownAttributesMask);
                    instance.m_bytesSent = BestValueOf(hasObytes64, obytes64, hasObytes, obytes, eBytesSent, instance.m_knownAttributesMask);
                    instance.m_bytesReceived = BestValueOf(hasRbytes64, rbytes64, hasRbytes, rbytes, eBytesReceived, instance.m_knownAttributesMask);
                    instance.m_errorsSending = ValueOf(hasOerrors, oerrors, eErrorsSending, instance.m_knownAttributesMask);
                    instance.m_errorsReceiving = ValueOf(hasIerrors, ierrors, eErrorsReceiving, instance.m_knownAttributesMask);
                    instance.m_collisions = ValueOf(hasCollisions, collisions, eCollisions, instance.m_knownAttributesMask);
                    instance.m_speed=ValueOf(hasIfspeed, ifspeed, eSpeed, instance.m_knownAttributesMask);;

                    // Save the kstat criteria values for searching other values
                    instance.m_ks_module=StrFromUTF8(cur->ks_module);
                    instance.m_ks_instance=cur->ks_instance;

                    interfaces.push_back(instance);
                }
                else
                {
                    SCX_LOGHYSTERICAL(m_log, StrAppend(
                                                 StrAppend(wstring(L"FindAllUsingKStat: Disqualified "),
                                                        StrFromUTF8(cur->ks_name)),
                                                 L" (no stats)"));
                }
            }
            else
            {
                SCX_LOGHYSTERICAL(m_log, StrAppend(
                                             StrAppend(wstring(L"FindAllUsingKStat: Disqualified "),
                                                       StrFromUTF8(cur->ks_name)),
                                             L" (ioctl failed)"));
            }
        }
    }

    SCX_LOGHYSTERICAL(m_log, L"NetworkInterfaceInfo::FindAllUsingKStat exit");
}

/*----------------------------------------------------------------------------*/
/**
    Get Attributes Using Kstat, like cap_autoneg
    \param       deps - dependency
*/
void NetworkInterfaceInfo::GetAttributesUsingKstat(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps)
{
    try
    {
        SCXCoreLib::SCXHandle<SCXKstat> kstat = deps->CreateKstat();
        // Look up the node according to ks_module, ks_name, and ks_instance;
        kstat->Lookup(m_ks_module, KSTAT_MII, m_ks_instance);

        for (kstat_t* cur = kstat->ResetInternalIterator(); cur; cur = kstat->AdvanceInternalIterator())
        {
            // Use mii as the ks_name to get the cap_autoneg
            if (StrFromUTF8(cur->ks_name).compare(KSTAT_MII) == 0 && StrFromUTF8(cur->ks_module).compare(m_ks_module) == 0 && cur->ks_instance == m_ks_instance)
            {
                if (cur->ks_type == KSTAT_TYPE_NAMED)
                {
                    scxulong autoneg=10000;
                    bool result = kstat->TryGetValue(StrFromUTF8(KSTAT_CAP_AUTONEG), autoneg);
                    if (result)
                    {
                        m_autoSense=autoneg>0?true:false;
                        m_knownAttributesMask |= eAutoSense;
                    }
                    break;
                }
            }
        }
    }
    catch (SCXCoreLib::SCXException& e)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eInfo, SCXCoreLib::eTrace);
        wstring msg(wstring(L"Unable to determine autosense attribute: ").append(e.What()));
        SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(msg));
        SCX_LOG(m_log, severity, msg);
    }
}

/*----------------------------------------------------------------------------*/
/**
    Parse Mac Addr using arp.
    \param fd file descriptor
    \param deps dependency
*/
void NetworkInterfaceInfo::ParseMacAddr(int fd, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps)
{
    struct ifreq ifr;
    struct arpreq arpreq;
    memset(&ifr, 0, sizeof (ifr));
    memset(&arpreq, 0, sizeof (arpreq));
    strncpy(ifr.ifr_name, StrToUTF8(m_name).c_str(), sizeof ifr.ifr_name);
    if (deps->ioctl(fd, SIOCGIFADDR, &ifr) >= 0)
    {
        ((struct sockaddr_in*)&arpreq.arp_pa)->sin_addr.s_addr=((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;
        if (deps->ioctl(fd,SIOCGARP,(char*)&arpreq) >= 0)
        {
            m_macAddress = FormatMacAddress((unsigned int)(unsigned char)arpreq.arp_ha.sa_data[0],
                                            (unsigned int)(unsigned char)arpreq.arp_ha.sa_data[1],
                                            (unsigned int)(unsigned char)arpreq.arp_ha.sa_data[2],
                                            (unsigned int)(unsigned char)arpreq.arp_ha.sa_data[3],
                                            (unsigned int)(unsigned char)arpreq.arp_ha.sa_data[4],
                                            (unsigned int)(unsigned char)arpreq.arp_ha.sa_data[5]);
            SCX_LOGINFO(m_log, StrAppend(L"Retreive MAC address : ", m_macAddress));
        }
    }
}
#endif //sun

#if defined(linux)
/*----------------------------------------------------------------------------*/
//! Find all network interfaces using the KStat API
//! \param[out]    interfaces          To be populated
//! \param[in]     deps                Dependencies to rely on
//! \throws        SCXErrnoException   KStat problems
void NetworkInterfaceInfo::FindAllInFile(std::vector<NetworkInterfaceInfo> &interfaces,
                                         SCXHandle<NetworkInterfaceDependencies> deps) {
    std::vector<wstring> lines;
    SCXStream::NLFs foundNlfs;
    SCXFile::ReadAllLines(deps->GetDynamicInfoFile(), lines, foundNlfs);
    for (size_t nr = 2; nr < lines.size(); nr++) {
        wistringstream infostream(lines[nr]);
        infostream.exceptions(std::ios::failbit | std::ios::badbit);
        wstring interface_name = ReadInterfaceName(infostream);

        // Skip the loopback interface (WI 463810)
        FileDescriptor fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, StrToUTF8(interface_name).c_str());

        // if not found with ioctl, don't add an instance and continue the loop
        if (deps->ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0)
        {
            if (ifr.ifr_ifru.ifru_flags & IFF_LOOPBACK)
                continue;
            
            NetworkInterfaceInfo instance(deps);
            instance.m_name = interface_name;
            scxlong skip;
            infostream >> instance.m_bytesReceived;
            instance.m_knownAttributesMask |= eBytesReceived;
            infostream >> instance.m_packetsReceived;
            instance.m_knownAttributesMask |= ePacketsReceived;
            infostream >> instance.m_errorsReceiving;
            instance.m_knownAttributesMask |= eErrorsReceiving;
            infostream >> skip;
            infostream >> skip;
            infostream >> skip;
            infostream >> skip;
            infostream >> skip;
            infostream >> instance.m_bytesSent;
            instance.m_knownAttributesMask |= eBytesSent;
            infostream >> instance.m_packetsSent;
            instance.m_knownAttributesMask |= ePacketsSent;
            infostream >> instance.m_errorsSending;
            instance.m_knownAttributesMask |= eErrorsSending;
            infostream >> skip;
            infostream >> skip;
            infostream >> instance.m_collisions;
            instance.m_knownAttributesMask |= eCollisions;

            interfaces.push_back(instance);
        } 
    }

}
#endif

#if defined(aix)
/*----------------------------------------------------------------------------*/
//! Find all network interfaces using the perfstat API
//! \param[out]    interfaces       To be populated
//! \param[in]     deps             Dependencies to rely on
//! \throws        SCXErrnoException   perfstat problems
void NetworkInterfaceInfo::FindAllUsingPerfStat(std::vector<NetworkInterfaceInfo> &interfaces,
                                             SCXHandle<NetworkInterfaceDependencies> deps) {

    perfstat_id_t first;
    int structsAvailable = deps->perfstat_netinterface(NULL, NULL, sizeof(perfstat_netinterface_t), 0);
    if (structsAvailable < 0) {
        throw SCXErrnoException(L"perfstat_netinterface", errno, SCXSRCLOCATION);
    }
    vector<char> buffer(structsAvailable * sizeof(perfstat_netinterface_t));
    perfstat_netinterface_t *statp = reinterpret_cast<perfstat_netinterface_t *>(&buffer[0]);
    strcpy(first.name, FIRST_NETINTERFACE);
    int structsReturned = deps->perfstat_netinterface(&first, statp, sizeof(perfstat_netinterface_t), structsAvailable);
    if (structsReturned < 0) {
        throw SCXErrnoException(L"perfstat_netinterface", errno, SCXSRCLOCATION);
    }

    for (int nr = 0; nr < structsReturned; nr++) {
        // Currently there is no way to return type of network, our current CIM-model supports ethernet
        if (statp[nr].type == IFT_ETHER) {
            NetworkInterfaceInfo instance(deps);
            instance.m_name = StrFromUTF8(statp[nr].name);

            instance.m_packetsSent = statp[nr].opackets;
            instance.m_knownAttributesMask |= ePacketsSent;

            instance.m_packetsReceived = statp[nr].ipackets;
            instance.m_knownAttributesMask |= ePacketsReceived;

            instance.m_bytesSent = statp[nr].obytes;
            instance.m_knownAttributesMask |= eBytesSent;

            instance.m_bytesReceived = statp[nr].ibytes;
            instance.m_knownAttributesMask |= eBytesReceived;

            instance.m_errorsSending = statp[nr].oerrors;
            instance.m_knownAttributesMask |= eErrorsSending;

            instance.m_errorsReceiving = statp[nr].ierrors;
            instance.m_knownAttributesMask |= eErrorsReceiving;

            instance.m_collisions = statp[nr].collisions;
            instance.m_knownAttributesMask |= eCollisions;

            interfaces.push_back(instance);
        }
    }

}


/*----------------------------------------------------------------------------*/
/**
    Parse Mac Addr using  getkerninfo. Also set NetworkAdapterType
    and AdapterTypeID
    \param fd file descriptor
*/
void NetworkInterfaceInfo::ParseMacAddrAix(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps)
{
    struct kinfo_ndd *nddp;
    size_t i;
    int size, nrec;
    char maddr[6];
    char *pMacAddr = &maddr[0];
    memset(pMacAddr, '\0', 6);
    bool succ = false;

    SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::ParseMacAddrAix entry");

    // First, let's get the size of the results:
    SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::ParseMacAddrAix getkerninfo KINFO_NDD");
    size = deps->getkerninfo(KINFO_NDD, 0, 0, 0);
    if (size <= 0)
    {
        SCX_LOGERROR(m_log, StrAppend(wstring(L"No MAC address available for "),m_name));
        m_macAddress.clear();
        return;
    }
    nrec = size / sizeof(struct kinfo_ndd);
    nddp = new struct kinfo_ndd[nrec];
    NDDMAP::const_iterator pos = m_mapNWAdaptTypeIdToNDDType.end();

    SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::ParseMacAddrAix getkerninfo KINFO_NDD for size");
    if (deps->getkerninfo(KINFO_NDD, (char *)nddp, &size, 0) >= 0)
    {
        // We have successfully retrieved the info

        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::ParseMacAddrAix Parsing results");
        for (i=0; i<nrec; i++)
        {
            wstring thisName(StrFromUTF8(nddp[i].ndd_name));
            wstring thisAlias(StrFromUTF8(nddp[i].ndd_alias));
            if ((m_name == thisName) ||
                (m_name == thisAlias))
            {
                memcpy(pMacAddr, nddp[i].ndd_addr, 6);
                pos = m_mapNWAdaptTypeIdToNDDType.find(nddp[i].ndd_type);

                if (pos == m_mapNWAdaptTypeIdToNDDType.end())
                {
                    m_adapterTypeID = eNetworkAdapterTypeInvalid;
                    m_adapterType = L"";
                }
                else
                {
                    m_adapterTypeID = pos->second;
                    m_adapterType = AdapterTypeNames[m_adapterTypeID];
                }

                succ = true;
                break;
            }
        }

        if (succ)
        {
            SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::ParseMacAddrAix Calling FormatMacAddress");
            m_macAddress = FormatMacAddress((unsigned int)(unsigned char)pMacAddr[0],
                                            (unsigned int)(unsigned char)pMacAddr[1],
                                            (unsigned int)(unsigned char)pMacAddr[2],
                                            (unsigned int)(unsigned char)pMacAddr[3],
                                            (unsigned int)(unsigned char)pMacAddr[4],
                                            (unsigned int)(unsigned char)pMacAddr[5]);
        }
    }
    else
    {
        SCX_LOGERROR(m_log, StrAppend(wstring(L"Failed to retrieve kerninfo for "),m_name));
        m_macAddress.clear();
    }

    if (nddp)
    {
        delete [] nddp;
    }

    return;
}

#endif

#if defined(hpux)
/*----------------------------------------------------------------------------*/
//! Find all network interfaces by interacting with the DLPI driver
//! \param[in]     baseName            Name except index
//! \param[out]    interfaces          To be populated
//! \param[in]     deps                Dependencies to rely on
void NetworkInterfaceInfo::FindAllInDLPI(std::vector<NetworkInterfaceInfo> &interfaces,
                         SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps) {

    SCXdlpi dlpi_instance(deps);
    std::vector<DLPIStatsEntry> statsVector = dlpi_instance.GetAllLANStats();
    
    for (std::vector<DLPIStatsEntry>::iterator i = statsVector.begin(); i != statsVector.end(); i++) {
        
        string namePPA;
        std::stringstream ppaStream;
        ppaStream << i->name << i->ppa;
        ppaStream >> namePPA;
        
        // if not found with ioctl, don't add an instance and continue the loop
        FileDescriptor fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, namePPA.c_str(), sizeof(ifr.ifr_name)-1);
        
        if (deps->ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
        {
            continue;
        }
        
        NetworkInterfaceInfo instance(deps);
        instance.m_name = StrFromUTF8(namePPA);
        
        instance.m_macAddress = FormatMacAddress(((unsigned char *)i->stats.ifPhysAddress.o_bytes)[0],
                                                 ((unsigned char *)i->stats.ifPhysAddress.o_bytes)[1],
                                                 ((unsigned char *)i->stats.ifPhysAddress.o_bytes)[2],
                                                 ((unsigned char *)i->stats.ifPhysAddress.o_bytes)[3],
                                                 ((unsigned char *)i->stats.ifPhysAddress.o_bytes)[4],
                                                 ((unsigned char *)i->stats.ifPhysAddress.o_bytes)[5]);
        
        instance.m_packetsSent = i->stats.ifOutUcastPkts + i->stats.ifOutNUcastPkts;
        instance.m_knownAttributesMask |= ePacketsSent;
        
        instance.m_packetsReceived = i->stats.ifInUcastPkts + i->stats.ifInNUcastPkts;
        instance.m_knownAttributesMask |= ePacketsReceived;
        
        instance.m_bytesSent = i->stats.ifOutOctets;
        instance.m_knownAttributesMask |= eBytesSent;
        
        instance.m_bytesReceived = i->stats.ifInOctets;
        instance.m_knownAttributesMask |= eBytesReceived;
        
        instance.m_errorsSending = i->stats.ifOutErrors;
        instance.m_knownAttributesMask |= eErrorsSending;
        
        instance.m_errorsReceiving = i->stats.ifInErrors;
        instance.m_knownAttributesMask |= eErrorsReceiving;
        
        instance.m_collisions = i->collisions;
        instance.m_knownAttributesMask |= eCollisions;
        
        interfaces.push_back(instance);
    }
    
}
#endif

#if defined(sun)
/*----------------------------------------------------------------------------*/
//! Constructs a kstat wrapper for the caller
//! \returns    a handle to the new kstat object
SCXCoreLib::SCXHandle<SCXKstat> NetworkInterfaceDependencies::CreateKstat()
{
    return SCXHandle<SCXKstat>(new SCXKstat());
}
#endif


#if defined(linux)
/*----------------------------------------------------------------------------*/
//! Retrieves the name of the file containing dynamic network interface properties
//! \returns    Path to file
SCXCoreLib::SCXFilePath NetworkInterfaceDependencies::GetDynamicInfoFile() const {
    return L"/proc/net/dev";
}

#endif

/*----------------------------------------------------------------------------*/
//! Perform a variety of control functions on devices
//! \param[in]     fildes    Open file descriptor
//! \param[in]     request   Control function selected
//! \param[in]     ifreqptr  Pointer to buffer to be initialized
int NetworkInterfaceDependencies::ioctl(int fildes, int request, void *ifreqptr) {
    return ::ioctl(fildes, request, ifreqptr);
}

#if defined(aix)
/*----------------------------------------------------------------------------*/
//! Find performance statistics for network interface
//! \param[in]   name    Either FIRST_NETINTERFACE, or the first network interface of interest
//! \param[in]   userbuff  Points to a memory area to be filled with structs
//! \param[in]   sizeof_struct Number of bytes of a struct
//! \param[in]   desiredNumber    Number of structs to fetch
//! \returns Number of structs copied to buffer, if name and userbuff are non null
//! If name or userbuff is NULL no structs are copied and the function returns the number of structs
//! that would have been copied.
int  NetworkInterfaceDependencies::perfstat_netinterface(perfstat_id_t *name, perfstat_netinterface_t *userbuff,
                                                         size_t sizeof_struct, int desiredNumber) {
    return ::perfstat_netinterface(name, userbuff, sizeof_struct, desiredNumber);
}

/*----------------------------------------------------------------------------*/
//! Perform a variety of control functions on devices
//! \param[in]     s   Socket file descriptor
//! \param[in]     name   Pointer to the socket structure
//! \param[in]     namelen  Length of the socket structure 
int NetworkInterfaceDependencies::bind(int s, const struct sockaddr *name, socklen_t namelen) {
    return ::bind(s, name, namelen);
}

/*----------------------------------------------------------------------------*/
//! Find Network Adapter Info such as MAC address, AdapterType, AdapterTypeID
//! \param[in]   func
//! \param[in]   kinfoStruct
//! \param[in]   totSize
//! \param[in]   args    Not used
//! \returns 0 for success, non-zero for error
int  NetworkInterfaceDependencies::getkerninfo(int func, char *kinfoStruct, int *totSize, int32long64_t args)
{
    return ::getkerninfo(func, kinfoStruct, totSize, args);
}
#endif // if defined(aix)

#if defined(hpux)
/*----------------------------------------------------------------------------*/
//! Default open function that gets overridden in HP unit tests
//! \param[in]   path    path of file to open 
//! \param[in]   oflag   flags specified for opening
//! \returns A file descriptor int associated with the file that is opened
int NetworkInterfaceDependencies::open(const char * path, int oflag) {
    return ::open(path, oflag);
}

/*----------------------------------------------------------------------------*/
//! Used in this implementation to get important data from the DLPI driver
//! \param[in]   fildes  the file descriptor that is mapped to an open file
//! \param[in]   ctlptr  a buffer to store control parts of messages
//! \param[in]   dataptr a buffer to store data parts of messages
//! \param[in]   flagsp  describes which messages should be retreived
//! \returns negative if error, MORECTL if there was not enough room in ctl ptr and 0 if successful
int NetworkInterfaceDependencies::getmsg(int fildes, struct ::strbuf * ctlptr, struct ::strbuf * dataptr, int * flagsp) {
    return ::getmsg(fildes, ctlptr, dataptr, flagsp);
}

/*----------------------------------------------------------------------------*/
//! Used in this implementation to request/send data to the DLPI driver
//! \param[in]   fildes  the file descriptor that is mapped to an open file
//! \param[in]   ctlptr  a buffer to store control parts of messages
//! \param[in]   dataptr a buffer to store data parts of messages
//! \param[in]   flags   describes which messages should be sent
//! \returns 0 if successful, -1 if unsuccessful
int NetworkInterfaceDependencies::putmsg(int fildes, const struct ::strbuf * ctlptr, const struct ::strbuf * dataptr, int flags) {
    return ::putmsg(fildes, ctlptr, dataptr, flags);
}
#endif

/*----------------------------------------------------------------------------*/
//! Make the information correspond to the current state of the system
void NetworkInterfaceInfo::Refresh() {
    vector<NetworkInterfaceInfo> latestInterfaces(FindAll(m_deps));
    for (size_t nr = 0; nr < latestInterfaces.size(); nr++) {
        if (latestInterfaces[nr].GetName() == GetName()) {
            *this = latestInterfaces[nr];
            break;
        }
    }

}

#if defined(linux)
    /*----------------------------------------------------------------------------*/
    //! parase data get by ioctl(fd, SIOCGIFHWADDR,), this function will get
    //! AdapterTypeID, AdapterType, PhysicalAdapter, MACAddress.
    //!
    //! \param fd file descriptor
    //! \param deps dependency
    //!
    void NetworkInterfaceInfo::ParseHwAddr(int fd, SCXHandle<NetworkInterfaceDependencies> deps)
    {
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(ifr));
        // set a invalid value
        m_adapterTypeID = eNetworkAdapterTypeInvalid;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, SCXCoreLib::StrToUTF8(m_name).c_str(), IFNAMSIZ - 1);
        if (deps->ioctl(fd, SIOCGIFHWADDR, &ifr) >= 0) {
            switch(ifr.ifr_hwaddr.sa_family)
            {
            case ARPHRD_ETHER:
                m_adapterTypeID = eNetworkAdapterTypeEthernet8023;
                break;
            case ARPHRD_FDDI:
                m_adapterTypeID = eNetworkAdapterTypeFDDI;
                break;
            case ARPHRD_LOCALTLK:
                m_adapterTypeID = eNetworkAdapterTypeLocalTalk;
                break;
            case ARPHRD_ARCNET:
                m_adapterTypeID = eNetworkAdapterTypeARCNET;
                break;
            case ARPHRD_ATM:
                m_adapterTypeID = eNetworkAdapterTypeATM;
                break;
            case ARPHRD_IEEE80211:
                m_adapterTypeID = eNetworkAdapterTypeWireless;
                break;
            case ARPHRD_IEEE1394:
                m_adapterTypeID = eNetworkAdapterType1394;
                break;
            default:
                // other values do not have correspond values defined in Win32_NetworkAdapter.
                static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eInfo, SCXCoreLib::eTrace);
                std::wstringstream errMsg;

                errMsg << L"For net device " << m_name << L", can not map sa_family to AdapterType, sa_family is: " << ifr.ifr_hwaddr.sa_family;
                SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(errMsg.str()));
                SCX_LOG(m_log, severity, errMsg.str());

                m_adapterTypeID = eNetworkAdapterTypeInvalid;
            }
            if (eNetworkAdapterTypeInvalid == m_adapterTypeID)
            {
                m_adapterType = L"";
            }
            else
            {
                m_adapterType = AdapterTypeNames[m_adapterTypeID];
            }
            /* in <linux/if_arp.h>, Dummy types for non ARP hardware start
             * at ARPHRD_SLIP, which is 256 */
            if (ifr.ifr_hwaddr.sa_family >= ARPHRD_SLIP)
            {
                m_physicalAdapter = false;
            }
            else
            {
                m_physicalAdapter = true;
            }
            m_knownAttributesMask |= ePhysicalAdapter;

            m_macAddress = FormatMacAddress((unsigned int)(unsigned char)ifr.ifr_hwaddr.sa_data[0],
                                            (unsigned int)(unsigned char)ifr.ifr_hwaddr.sa_data[1],
                                            (unsigned int)(unsigned char)ifr.ifr_hwaddr.sa_data[2],
                                            (unsigned int)(unsigned char)ifr.ifr_hwaddr.sa_data[3],
                                            (unsigned int)(unsigned char)ifr.ifr_hwaddr.sa_data[4],
                                            (unsigned int)(unsigned char)ifr.ifr_hwaddr.sa_data[5]);
        }
        else
        {
            SCX_LOGERROR(m_log, L"for net device " + m_name + L" ioctl(,SIOCGIFHWADDR,) fail : " + wstrerror(errno));
            m_macAddress.clear();
        }
    }
    /*----------------------------------------------------------------------------*/
    //! parse data get by ioctl(fd, SIOETHTOOL, ), will get attribute
    //! AutoSense, MaxSpeed, Speed
    //!
    //! \param fd file descriptor
    //! \param deps dependency
    //!
    void NetworkInterfaceInfo::ParseEthtool(int fd, SCXHandle<NetworkInterfaceDependencies> deps)
    {
        struct ifreq       ifr;
        struct ethtool_cmd ecmd;

        ecmd.cmd = ETHTOOL_GSET; /* get setting */
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, SCXCoreLib::StrToUTF8(m_name).c_str(), IFNAMSIZ - 1);
        ifr.ifr_data = (caddr_t) &ecmd;

        m_autoSense = false;
        if (deps->ioctl(fd, SIOCETHTOOL, &ifr) >= 0)
        {
            /* macros defined in file <linux/ethtool.h> */
            if ((ecmd.supported & SUPPORTED_Autoneg) &&
               (AUTONEG_ENABLE == ecmd.autoneg))
            {
                m_autoSense = true;
            }
            else
            {
                m_autoSense = false;
            }
            m_knownAttributesMask |= eAutoSense;
            m_maxSpeed = 0;
#if defined(SUPPORTED_10000baseT_Full) || defined(SUPPORTED_10000baseKX4_Full) || defined(SUPPORTED_10000baseKR_Full) || defined(SUPPORTED_10000baseR_FEC)
            if ((ecmd.supported & SUPPORTED_10000baseT_Full)
#ifdef SUPPORTED_10000baseKX4_Full
               || (ecmd.supported & SUPPORTED_10000baseKX4_Full)
#endif
#ifdef SUPPORTED_10000baseKR_Full
               || (ecmd.supported & SUPPORTED_10000baseKR_Full)
#endif
#ifdef SUPPORTED_10000baseR_FEC
               || (ecmd.supported & SUPPORTED_10000baseR_FEC)
#endif
                )
            {
                m_maxSpeed = SPEED_10000;
            }
#endif
#ifdef SUPPORTED_2500baseX_Full
            if (m_maxSpeed < SPEED_2500 && (ecmd.supported & SUPPORTED_2500baseX_Full))
            {
                m_maxSpeed = SPEED_2500;
            }
#endif
            if (m_maxSpeed < SPEED_1000 &&
               ((ecmd.supported & SUPPORTED_1000baseT_Full) ||
#if defined(SUPPORTED_1000baseKX_Full)
                (ecmd.supported & SUPPORTED_1000baseKX_Full) ||
#endif
                (ecmd.supported & SUPPORTED_1000baseT_Half)))
            {
                m_maxSpeed = SPEED_1000;
            }
            else if ((ecmd.supported & SUPPORTED_100baseT_Full) ||
                    (ecmd.supported & SUPPORTED_100baseT_Half))
            {
                m_maxSpeed = SPEED_100;
            }
            else if ((ecmd.supported & SUPPORTED_10baseT_Full) ||
                    (ecmd.supported & SUPPORTED_10baseT_Half))
            {
                m_maxSpeed = SPEED_10;
            }
            else
            {
                SCX_LOGTRACE(m_log, StrAppend(wstring(L"for net device ") + m_name + L" can not get supported speed, the supported value got by ioctl(,SIOCETHTOOL,) is : ", ecmd.supported));
                m_maxSpeed = 0;
            }
            m_maxSpeed *= 1000 * 1000; // change speed from Mbits to bits
            switch(ecmd.speed)
            {                     // macroes defined in <linux/ethtool.h>
                case SPEED_10:    // 10Mb
                case SPEED_100:   // 100Mb
                case SPEED_1000:  // gigabit
#ifdef SPEED_2500
                case SPEED_2500:  // 2.5Gb
#endif
#ifdef SPEED_10000
                case SPEED_10000: // 10GbE
#endif
                    m_speed = ecmd.speed * 1000 * 1000; // change speed from Mbits to bits
                    break;
            default:            /* don't support */
                SCX_LOGTRACE(m_log, StrAppend(wstring(L"for net device ") + m_name + L" ioctl(,SIOCETHTOOL,) get a unformal speed value : ", ecmd.speed))
                m_speed = 0;
            }
        }
        else
        {
            SCX_LOGTRACE(m_log, L"for net device " + m_name + L" ioctl(,SIOCETHTOOL,) fail : " + wstrerror(errno));
        }
    }
#endif

#if defined(hpux)
    void NetworkInterfaceInfo::Get_DataLink_Speed(SCXHandle<NetworkInterfaceDependencies> deps)
    {
        dl_hp_get_drv_param_ioctl_t cmd_info; // To store reported values by DLPI.
        SCXdlpi dlpi_instance(deps);

        m_autoSense = false;
        m_speed = 0;
        if (dlpi_instance.get_cur_link_speed(m_name, cmd_info))
        {
            if (cmd_info.dl_autoneg == DL_HP_AUTONEG_SENSE_ON)
            {
                m_autoSense = true;
            }
            else
            {
                m_autoSense = false;
            }
            m_knownAttributesMask |= eAutoSense;
            
            // Provider needs the speed in Bytes/Sec.
            m_speed = cmd_info.dl_speed * 1000 * 1000;
        }
        else
        {
            SCX_LOGERROR(m_log, std::wstring(L"for net deivce ").append(SCXCoreLib::StrFrom(m_name)).append(std::wstring(L" and errno=")).append(SCXCoreLib::StrFrom(errno)));
        }
    }
#endif // if defined(hpux)

#if defined(aix)
static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);

    /** AutoClose destructor */
    NetworkInterfaceInfo::AutoClose::~AutoClose()
    {
        if (m_fd != 0)
        {
            SCX_LOGHYSTERICAL(m_log, StrAppend(wstring(L"GetParameters: AutoClose closing fd: "), m_fd));
            if (close(m_fd) < 0)
            {
                SCX_LOGERROR(m_log, StrAppend(StrAppend(L"Error in ~AutoClose closing fd: ", m_fd), StrAppend(L", errno: ", SCXCoreLib::StrFrom(errno))));
            }
            m_fd = 0;
        }
    } // End of ~AutoClose().
    
    // Helper function for Get_NDD_STAT() function.
    void NetworkInterfaceInfo::set_speed(const scxulong speed_selected, 
                                         const scxulong auto_speed)
    {
        std::wstringstream errMsg(L"");
        switch(speed_selected)
        {
            case MEDIA_10_HALF:
            case MEDIA_10_FULL:
                m_speed = SPEED_10;
                break;

            case MEDIA_100_HALF:
            case MEDIA_100_FULL:
                m_speed = SPEED_100;
                break;

            case MEDIA_1000_FULL:
                m_speed = SPEED_1000;
                break;

            case MEDIA_AUTO:
                m_autoSense = true;
                switch(auto_speed)
                {
                    case MEDIA_10_HALF:
                    case MEDIA_10_FULL:
                        m_speed = SPEED_10;
                        break;

                    case MEDIA_100_HALF:
                    case MEDIA_100_FULL:
                        m_speed = SPEED_100;
                        break;

                    case MEDIA_1000_FULL:
                        m_speed = SPEED_1000;
                        break;

                    default:
                        errMsg << L"Invalid auto speed: " << auto_speed << L"- interface: " << m_name;
                        SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(SCXCoreLib::StrFrom(errMsg.str())));
                        SCX_LOG(m_log, severity, errMsg.str());
                        break;
                } // switch(auto_speed)
                break;

            default:
                errMsg << L"Invalid selected speed: " << speed_selected << L"- interface: " << m_name;
                SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(SCXCoreLib::StrFrom(errMsg.str())));
                SCX_LOG(m_log, severity, errMsg.str());
                break;
        }
    }
     
    void NetworkInterfaceInfo::Get_NDD_STAT(SCXHandle<NetworkInterfaceDependencies> deps)
    {
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::Get_NDD_STAT entry");

       /* This function provides the support for those drivers which report 
        * speed related stats as part of configuration parameters.
        * (defined in header files: /usr/include/sys/cdli_*.h) 
        *
        * The list of supported drivers is as follows: 
        * kent_config for the PCI Ethernet Device Driver (22100020)
        * phxent for the 10/100 Mbps Ethernet PCI Adapter Device Driver (23100020)
        * scent for the 10/100 Mbps Ethernet PCI Adapter II Device Driver (1410ff01)
        * gxent for the Gigabit Ethernet-SX PCI Adapter Device Driver (14100401)
        * goent for Gigabit Ethernet-SX PCI-X Adapter Device Driver (14106802), 
        *   10/100/1000 Base-T Ethernet PCI-X Adapter Device Driver (14106902), 
        *   2-Port Gigabit Ethernet-SX PCI-X Adapter Device Driver (14108802), 
        *   2-Port 10/100/1000 Base-TX PCI-X Adapter Device Driver (14108902), 
        *   4-Port 10/100/1000 Base-TX PCI-X Adapter Device Driver (14101103), 
        *   4-Port 10/100/1000 Base-TX PCI-Exp Adapter Dev Driver(14106803), 
        *   2-Port Gigabit Ethernet-SX PCI-Express Adapter Device Driver
        *                                                     (14103f03), 
        *   2-Port 10/100/1000 Base-TX PCI-Express Adapter Device Driver 
        *                                                     (14104003).
        *
        * ment  Gigabit Ethernet-SX PCI-X Adapter Device Driver (14106703).
        *
        * hea for Host Ethernet Adapter Device Driver.
        *********************END OF SUPPORTED DRIVERS ************************************/

       /* This function doesn't provide the support for the following drivers 
        * due to the overlap of their device_type values with some of the 
        * above list drivers:
        * 
        * bent for the Gigabit Ethernet-SX Adapter Device Driver (e414a816).
        *  (reason: ENT_BT_PCI, ENT_UTP_PCI, ENT_BT_PCI_OTHER,and ENT_UTP_PCI_OTHER 
        *  have the same value as ENT_CENT_PCI_TX,ENT_UTP_PCI,
        *  ENT_GX_PCI_OTHER, ENT_UTP_PCI_OTHER(defined for gxent) respectively
        *
        * ment for Gigabit Ethernet-SX Adapter Device Driver (14101403)
        *   (reason: ENT_MT_SX_PCI has the same value as ENT_GX_PCI defined for gxent)
        *
        * kngent for the 10 Gigabit Ethernet-SR PCI-X 2.0 DDR Adapter Device 
        * Driver (1410eb02) and the 10 Gigabit Ethernet-LR PCI_X 2.0 DDR 
        * Adapter Device Driver (1410ec02).
        * (reason: ENT_KNGENT_LR_PCIX and ENT_KNGENT_SR_PCIX have the same value as 
        *  ENT_CENT_PCI_TX and ENT_EPENT_PCI_TX respectively (defined for 
        *  goent driver))
        */

        union ARG 
        {
            kent_all_stats_t kent;
            phxent_all_stats_t phxent;
            scent_all_stats_t scent;
            gxent_all_stats_t gxent;
            goent_all_stats_t goent;
            ment_all_stats_t ment;
            hea_all_stats_t hea;
            lncent_all_stats_t lncent;
            shient_all_stats_t shient;
        } arg; // Used for ioctl argument. 

        // First, we need to connect to the adapter in question.
        struct sockaddr_ndd_8022 sa;
        int s;

        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::Get_NDD_STAT Connecting to socket");
        s = socket(AF_NDD, SOCK_DGRAM, 0);
        if (s < 0)
        {
            std::wstringstream errMsg;
            errMsg.str(L"");
            errMsg << L"socket(AF_NDD,SOCK_DGRAM,0) failed. errno: " << errno << L"- interface: " << m_name;
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(SCXCoreLib::StrFrom(errMsg.str())));
            SCX_LOG(m_log, severity, errMsg.str());
            return;
        }

        // Close the resource through a helper class, should an exception happens. 
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::Get_NDD_STAT Setting up AutoClose");
        AutoClose _fd(m_log, s);

        sa.sndd_8022_family = AF_NDD;
        sa.sndd_8022_len = sizeof(struct sockaddr_ndd_8022);
        sa.sndd_8022_filtertype = NS_TAP;
        sa.sndd_8022_filterlen = sizeof(ns_8022_t);
        strcpy((char *)sa.sndd_8022_nddname, SCXCoreLib::StrToUTF8(m_name).c_str());

        SCX_LOGTRACE(m_log, wstring(L"NetworkInterfaceInfo::Get_NDD_STAT Binding to socket") + m_name);
        if (deps->bind(s, (struct sockaddr *)&sa, sizeof(struct sockaddr_ndd_8022)) < 0) 
        {
            // This is to log file name and line number along with the rest of error message.
            SCXCoreLib::SCXErrnoException e(L"bind() failed. errno: ", errno, SCXSRCLOCATION);
            SCX_LOGERROR(m_log, e.What());
            return;
        }

        int on = 1;
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::Get_NDD_STAT Setting option SO_REUSEADDR");
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        {
            SCXCoreLib::SCXErrnoException e(L"setsockopt() failed. errno: ", errno, SCXSRCLOCATION);
            SCX_LOGERROR(m_log, e.What());
            return;
        }

        // Populate the ioctl argument accordingly. 
        // The ioctl argument for the stat related commands must be struct nddctl.
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::Get_NDD_STAT Populating ioctl");
        nddctl ioctl_arg;
        ioctl_arg.nddctl_buflen = sizeof(ARG);
        ioctl_arg.nddctl_buf = (caddr_t)&arg;

        // Issue the ioctl command to get the device extended stats.
        // (http://pic.dhe.ibm.com/infocenter/aix/v6r1/index.jsp?topic=%2Fcom.ibm.aix.kernelext%2Fdoc%2Fkernextc%2Fndd_get_all_stats_devctrlop.htm)
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::Get_NDD_STAT Issuing ioctl");
        if (deps->ioctl(s,NDD_GET_ALL_STATS,&ioctl_arg) < 0) 
        {
            SCXCoreLib::SCXErrnoException e(L"ioctl(s,NDD_GET_ALL_STATS,&arg) failed. errno: ", errno, SCXSRCLOCATION);
            SCX_LOGERROR(m_log, e.What());
            return;
        }
        m_autoSense = false;
        m_speed = 0;
        m_maxSpeed = 0;
        unsigned int device_type = arg.kent.ent_gen_stats.device_type;
        scxulong auto_speed = 0; // meida_speed, speed_negotiated, or link_negotiated.

        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::Get_NDD_STAT At device switch");

        // Find out which driver we are dealing with. 
        // If supported then retrieve the intended info.
        // (http://pic.dhe.ibm.com/infocenter/aix/v6r1/index.jsp?topic=%2Fcom.ibm.aix.kernelext%2Fdoc%2Fkernextc%2Fconfig.htm)
        switch (device_type)
        {
            // kent 
            case ENT_3COM:
            case ENT_IENT:
            case ENT_IEN_ISA:
            case ENT_LCE:
            case ENT_KEN_PCI:
            case ENT_LSA:
            case ENT_IEN_PCMCIA:
                // PCI Ethernet Adapter Device Driver (22100020) only supports the 
                // following additional configuration parameters
                // (speed is not one of them!):
                // Full Duplex, Hardware Transmit Queue, Hardware Receive Queue.
                m_maxSpeed = SPEED_100; // This is a 10/100Mbps Eth PCI adapter.
                break;

                // phxent
            case ENT_PHX_PCI:
            case ENT_CLVR_PCI:
            case ENT_PHX_INT_PCI:
            case ENT_CLVR_INT_PCI:
                m_maxSpeed = SPEED_100; // This is a 10/100Mbps Eth PCI adapter.
                set_speed(arg.phxent.phxent_stats.speed_selected, arg.phxent.phxent_stats.media_speed);
                break;

                // scent
            case ENT_SCENT_PCI:
                m_maxSpeed = SPEED_100; // This is a 10/100Mbps Eth PCI adapter.
                set_speed(arg.scent.scent_stats.speed_selected, arg.scent.scent_stats.speed_negotiated);
                break;

                // gxent
            case ENT_GX_PCI:
            case ENT_UTP_PCI:
            case ENT_GX_PCI_OTHER:
            case ENT_UTP_PCI_OTHER:
                m_maxSpeed = SPEED_1000; // This is a Gigabit Ethernet PCI adapter.
                auto_speed = arg.gxent.gxent_stats.link_negotiated;
                if (auto_speed & NDD_GXENT_LNK_10MB)
                {
                    auto_speed = MEDIA_10_FULL;
                }
                else if (auto_speed & NDD_GXENT_LNK_100MB) 
                {
                    auto_speed = MEDIA_100_FULL;
                }
                else if (auto_speed & NDD_GXENT_LNK_1000MB) 
                {
                    auto_speed = MEDIA_1000_FULL;
                }
                set_speed(arg.gxent.gxent_stats.speed_selected, auto_speed);
                break; 

                // goent
            case ENT_GOENT_PCI_TX:
            case ENT_GOENT_PCI_SX:
            case ENT_DENT_PCI_TX:
            case ENT_DENT_PCI_SX:
            case ENT_CENT_PCI_TX:
            case ENT_EPENT_PCI_TX:
            case ENT_EPENT_PCI_SX:
            case ENT_CLENT_PCI_TX:
                m_maxSpeed = SPEED_1000; // This is a Gigabit Ethernet PCI adapter.
                set_speed(arg.goent.goent_stats.speed_selected, arg.goent.goent_stats.speed_negotiated);
                break;

                // ment
            case ENT_SM_SX_PCI:
                m_maxSpeed = SPEED_1000; // This is a Gigabit Ethernet-SX Adapter. 
                auto_speed = arg.ment.ment_stats.link_negotiated;
                if (auto_speed & NDD_MENT_LNK_10MB)
                {
                    auto_speed = MEDIA_10_FULL;
                }
                else if (arg.ment.ment_stats.link_negotiated & NDD_MENT_LNK_100MB)
                {
                    auto_speed = MEDIA_100_FULL;
                }
                else if (arg.ment.ment_stats.link_negotiated & NDD_MENT_LNK_1000MB)
                {
                    auto_speed = MEDIA_1000_FULL;
                }
                set_speed(arg.ment.ment_stats.speed_selected, auto_speed);
                break;

                // lncent 
            case ENT_LNC_TYPE:
            case ENT_LNC_VF:
                //Support for Lancer drivers
                m_maxSpeed = SPEED_10000; // This is 10 Gigabit Ethernet PCI adapter.
                break;

            default:
                // Is it a Host Ethernet Adapter?
                // The reason I am using the sizeof() to indentify 
                // the HEA device type, is that there is no device type for it.
                // The ndd_2_flags in ndd_t structure represents the HEA device
                // type. I couldn't find a way to query the ndd_t structure. 
                // (I tried the NDD_GET_NDD command defined in ndd.h but 
                // it didn't work!

                if (ioctl_arg.nddctl_buflen = sizeof(hea_all_stats_t))
                {
                    switch(arg.hea.hea_stats.speed_selected)
                    {
                        case HEA_MEDIA_10_HALF:
                        case HEA_MEDIA_10_FULL:
                            m_speed = m_maxSpeed = SPEED_10;
                            break;

                        case HEA_MEDIA_100_HALF:
                        case HEA_MEDIA_100_FULL:
                            m_speed = m_maxSpeed = SPEED_100;
                            break;

                        case HEA_MEDIA_1000_FULL:
                            m_speed = m_maxSpeed = SPEED_1000;
                            break;

                        case HEA_MEDIA_10000_FULL:
                            m_speed = m_maxSpeed = SPEED_10000;
                            break;

                        case HEA_MEDIA_AUTO:
                            m_autoSense = true;
                            break;

                        default:
                            std::wstringstream errMsg;
                            errMsg.str(L"");
                            errMsg << L"Invalid seleced speed: " << arg.hea.hea_stats.speed_selected << L"- interface: " << m_name;
                            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(SCXCoreLib::StrFrom(errMsg.str())));
                            SCX_LOG(m_log, severity, errMsg.str());
                            break;
                    }
                }
                else // Not supported driver.
                {
                    std::wstringstream errMsg;
                    errMsg.str(L"");
                    errMsg << L"The driver not supported for the interface: " << m_name << " with device type: " << device_type;
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(SCXCoreLib::StrFrom(errMsg.str())));
                    SCX_LOG(m_log, severity, errMsg.str());
                }
                break;
        }

        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::Get_NDD_STAT After device switch");

        m_knownAttributesMask |= eAutoSense;

    } // End of Get_NDD_STAT()

#endif // if defined(aix)

#if defined(sun) || defined(linux)
    /*----------------------------------------------------------------------------*/
    //! parse data get by ioctl(fd, SIOCGIFINDEX, ), will get InterfaceIndex.The linux and solaris share the same method
    //!
    //! \param fd file descriptor
    //! \param deps dependency
    //!
    void NetworkInterfaceInfo::ParseIndex(int fd, SCXHandle<NetworkInterfaceDependencies> deps)
    {
        struct ifreq ifr;

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, SCXCoreLib::StrToUTF8(m_name).c_str(), IFNAMSIZ - 1);
        ifr.ifr_addr.sa_family = AF_INET;
        if (deps->ioctl(fd, SIOCGIFINDEX, &ifr) >= 0)
        {
#if defined(linux)
            m_interfaceIndex       = ifr.ifr_ifindex;
#elif defined(sun)
            m_interfaceIndex       = ifr.ifr_index;
#endif
            m_knownAttributesMask |= eInterfaceIndex;
        }
        else
        {
            SCX_LOGERROR(m_log, L"for net device " + m_name + L" ioctl(,SIOCGIFINDEX,) fail : " + wstrerror(errno));
        }
    }
#endif

/*----------------------------------------------------------------------------*/
//! Finds IPv6 addresses. 
//! \param[in]  deps    What this PAL depends on.
    void NetworkInterfaceInfo::ParseIPv6Addr(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps)
    {
#if defined(linux)
        class AutoIFAddr
        {
            struct ifaddrs * m_ifAddr;
            SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> m_deps;
        public:
            AutoIFAddr(struct ifaddrs * ifAddr, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> depsInit):
                    m_ifAddr(ifAddr), m_deps(depsInit)
            {
            }
            ~AutoIFAddr()
            {
                if(m_ifAddr != NULL)
                {
                    m_deps->freeifaddrs(m_ifAddr);
                }
            }
            struct ifaddrs * GetIFAddr()
            {
                return m_ifAddr;
            }
        };

        struct ifaddrs *ifAddrPtr;
        if (deps->getifaddrs(&ifAddrPtr) != 0)
        {
            SCX_LOGTRACE(m_log, L"For net device " + m_name + L" getifaddrs() failed, errno : " + wstrerror(errno) + L'.');
            return;
        }
        AutoIFAddr ifAddr(ifAddrPtr, m_deps);

        struct ifaddrs * ifa = NULL;
        void * pTmpAddr = NULL;

        for (ifa = ifAddr.GetIFAddr(); ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6 && strcmp(ifa->ifa_name, StrToUTF8(m_name).c_str()) == 0)
            { 
                pTmpAddr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
                char addrStr[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, pTmpAddr, addrStr, INET6_ADDRSTRLEN);
                m_ipv6Address.push_back(StrFromUTF8(addrStr));
            }
        }
#endif
#if defined(sun) || defined(hpux) || defined(aix)
        class AutoSocket
        {
            wstring m_devName;
            int m_sock;
            SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> m_deps;
            SCXCoreLib::SCXLogHandle m_log;
        public:
            AutoSocket(int sock, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> depsInit,
                    SCXCoreLib::SCXLogHandle log, const wstring &devName):
                    m_sock(sock), m_deps(depsInit), m_log(log), m_devName(devName)
            {
            }
            ~AutoSocket()
            {
                if(m_sock != -1)
                {
                    if(m_deps->close(m_sock) != 0)
                    {
                        SCX_LOGERROR(m_log, L"For net device " + m_devName + L" closing socket failed, errno : " +
                            wstrerror(errno) + L'.');
                    }
                }
            }
            int GetSock()
            {
                return m_sock;
            }
        };
        
        AutoSocket sd(deps->socket(AF_INET6, SOCK_DGRAM, 0), m_deps, m_log, m_name);
        if(sd.GetSock() == -1)
        {
            SCX_LOGTRACE(m_log, L"For net device " + m_name +
                L" opening socket(AF_INET6, SOCK_DGRAM, 0) failed, errno : " + wstrerror(errno) + L'.');
            return;
        }

#if defined(sun)
        int ifCnt = 0;
        struct lifnum lifn;
        lifn.lifn_family = AF_UNSPEC;
        lifn.lifn_flags = 0;
        if (deps->ioctl(sd.GetSock(), SIOCGLIFNUM, &lifn) != 0)
        {
            SCX_LOGTRACE(m_log, L"For net device " + m_name + L" ioctl(SIOCGLIFNUM) failed, errno : " +
                wstrerror(errno) + L'.');
            return;
        }
        ifCnt = lifn.lifn_count;

        if (ifCnt == 0)
        {
            // Nothing to do, and also later we count on the vector size to be more than 0.
            return;
        }

        vector<lifreq> lifcBuff;
        struct lifconf lifc;
        lifcBuff.resize(ifCnt);
        lifc.lifc_len = ifCnt * sizeof (lifreq);
        lifc.lifc_buf = (caddr_t)&lifcBuff[0];// It's safe because size of the vector is always more than 0.
        lifc.lifc_family = AF_UNSPEC;
        lifc.lifc_flags = 0;
        if (deps->ioctl(sd.GetSock(), SIOCGLIFCONF, &lifc) != 0)
        {
            SCX_LOGTRACE(m_log, L"For net device " + m_name + L" ioctl(SIOCGLIFCONF) failed, errno : " +
                wstrerror(errno) + L'.');
            return;
        }

        size_t lifrCnt = lifc.lifc_len / sizeof (lifreq);
        for (size_t i = 0; i < lifrCnt; i++)
        {
            string currName = lifcBuff[i].lifr_name;
            string name = SCXCoreLib::StrToUTF8(m_name);
            string name1 = name + ':';
            if (currName == name || currName.substr(0, name1.size()) == name1)
            {
                struct sockaddr * sockAddr = (struct sockaddr *)&lifcBuff[i].lifr_addr;
                if (sockAddr->sa_family == AF_INET6)
                {
                    char addrStr[INET6_ADDRSTRLEN];
                    inet_ntop(sockAddr->sa_family, &((struct sockaddr_in6 *)sockAddr)->sin6_addr, addrStr, INET6_ADDRSTRLEN);
                    m_ipv6Address.push_back(StrFromUTF8(addrStr)); 
                }
            }
        }
#elif defined(hpux)
        int ifCnt = 0;
        if (deps->ioctl(sd.GetSock(), SIOCGLIFNUM, &ifCnt) != 0)
        {
            SCX_LOGTRACE(m_log, L"For net device " + m_name + L" ioctl(SIOCGLIFNUM) failed, errno : " +
                wstrerror(errno) + L'.');
            return;
        }

        if (ifCnt == 0)
        {
            // Nothing to do, and also later we count on the vector size to be more than 0.
            return;
        }
        vector<if_laddrreq> lifcBuff;
        struct if_laddrconf lifc;
        lifcBuff.resize(ifCnt);
        lifc.iflc_len = ifCnt * sizeof (if_laddrreq);
        lifc.iflc_buf = (caddr_t)&lifcBuff[0];// It's safe because size of the vector is always more than 0.
        if (deps->ioctl(sd.GetSock(), SIOCGLIFCONF, &lifc) != 0)
        {
            SCX_LOGTRACE(m_log, L"For net device " + m_name + L" ioctl(SIOCGLIFCONF) failed, errno : " +
                wstrerror(errno) + L'.');
            return;
        }

        size_t lifrCnt = lifc.iflc_len / sizeof (if_laddrreq);
        for (size_t i = 0; i < lifrCnt; i++)
        {
            string currName = lifcBuff[i].iflr_name;
            string name = SCXCoreLib::StrToUTF8(m_name);
            string name1 = name + ':';
            if (currName == name || currName.substr(0, name1.size()) == name1)
            {
                struct sockaddr * sockAddr = (struct sockaddr *)&lifcBuff[i].iflr_addr;
                if (sockAddr->sa_family == AF_INET6)
                {
                    char addrStr[INET6_ADDRSTRLEN];
                    // We cast sockAddr to char* to stop the warning on RISC machines: sockaddr_in6 more strictly
                    // alligned than sockAddr. Addresses must be right anyway since they're returned by the previous
                    // ioctl(SIOCGLIFCONF) call.
                    inet_ntop(sockAddr->sa_family, &((struct sockaddr_in6 *)((char*)sockAddr))->sin6_addr, addrStr,
                        INET6_ADDRSTRLEN);
                    m_ipv6Address.push_back(StrFromUTF8(addrStr)); 
                }
            }
        }
#elif defined(aix)
// Code inspired by the IBM examples on how to use network system calls.
#if !defined(MAX)
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#endif
#define SIZE(p) MAX((p).sa_len, sizeof(p))
        int buffSize = 0;
        if (deps->ioctl(sd.GetSock(), SIOCGSIZIFCONF, &buffSize) != 0)
        {
            SCX_LOGTRACE(m_log, L"For net device " + m_name + L" ioctl(SIOCGSIZIFCONF) failed, errno : " +
                wstrerror(errno) + L'.');
            return;
        }

        vector<char> ifconfBuf;
        ifconfBuf.resize(buffSize);
        struct ifconf ifc;
        ifc.ifc_buf = &ifconfBuf[0];
        ifc.ifc_len = ifconfBuf.size();
        if (deps->ioctl(sd.GetSock(), SIOCGIFCONF, &ifc) != 0)
        {
            SCX_LOGTRACE(m_log, L"For net device " + m_name + L" ioctl(SIOCGIFCONF) failed, errno : " +
                wstrerror(errno) + L'.');
            return;
        }

        char *cp, *cplim;
        struct ifreq *ifr = ifc.ifc_req;
        cp = (char *)ifc.ifc_req;
        cplim = cp + ifc.ifc_len;
        // Iterate through the sequence of variable size data structures containing the interface name. cplim points to
        // the end of the sequence. Each data structure consists of the fixed size name array and variable size
        // additional data whose size is obtained by using the SIZE macro.
        for(; cp < cplim; cp += (sizeof(ifr->ifr_name) + SIZE(ifr->ifr_addr)))
        {
            ifr = (struct ifreq *)cp;
            if (0 == strcmp(ifr->ifr_name, StrToUTF8(m_name).c_str()))
            {
                // Interface name matches.
                struct sockaddr *sa;
                sa = (struct sockaddr *)&(ifr->ifr_addr);
                if (sa->sa_family == AF_INET6)
                {
                    // It is IPv6 address.
                    char addrStr[INET6_ADDRSTRLEN];
                    // Translate address to string.
                    inet_ntop(AF_INET6, (struct in6_addr *)&(((struct sockaddr_in6 *)sa)->sin6_addr), addrStr,
                        INET6_ADDRSTRLEN);
                    // Store the address.
                    m_ipv6Address.push_back(StrFromUTF8(addrStr));
                }
            }
        }
#endif
#endif// defined(sun) || defined(hpux) || defined(aix)
}

/*----------------------------------------------------------------------------*/
//! Find all network interfaces on the machine
//! \param[in]  deps    What this PAL depends on
//! \param[in]  includeNonRunning    If false find only interfaces that are up and running
//! \returns    Information on the network instances
std::vector<NetworkInterfaceInfo> NetworkInterfaceInfo::FindAll(SCXHandle<NetworkInterfaceDependencies> deps,
                                                                bool includeNonRunning /*= false*/) {
    SCXCoreLib::SCXLogHandle m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.networkinterface"));

    SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll entry");
    std::vector<NetworkInterfaceInfo> interfaces;
#if defined(linux)
    FindAllInFile(interfaces, deps);
#elif defined(sun)
    FindAllUsingKStat(interfaces, deps);
#elif defined(hpux)
    FindAllInDLPI(interfaces, deps);
#elif defined(aix)
    SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Calling FindAllUsingPerfStat");
    FindAllUsingPerfStat(interfaces, deps);
#else
#error "Platform not supported"
#endif
    SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Getting attributes for instance");
    FileDescriptor fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    for (size_t nr = 0; nr < interfaces.size(); nr++) {
        NetworkInterfaceInfo &instance = interfaces[nr];
        strcpy(ifr.ifr_name, StrToUTF8(instance.m_name).c_str());
        SCX_LOGTRACE(m_log, wstring(L"NetworkInterfaceInfo::FindAll working on interface ") + instance.m_name);

        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Attribute SIOCGIFADDR");
        if (deps->ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
            instance.m_ipAddress = ToString(ifr.ifr_addr);
            instance.m_knownAttributesMask |= eIPAddress;
        }
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Attribute SIOCGIFNETMASK");
        if (deps->ioctl(fd, SIOCGIFNETMASK, &ifr) >= 0) {
            instance.m_netmask = ToString(ifr.ifr_addr);
            instance.m_knownAttributesMask |= eNetmask;
        }
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Attribute SIOCGIFBRDADDR");
        if (deps->ioctl(fd, SIOCGIFBRDADDR, &ifr) >= 0) {
            instance.m_broadcastAddress = ToString(ifr.ifr_addr);
            instance.m_knownAttributesMask |= eBroadcastAddress;
        }
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Attribute SIOCGIFMTU");
        if (deps->ioctl(fd, SIOCGIFMTU, &ifr) >=0)
        {
#if defined(hpux) && PF_MAJOR == 11 && PF_MINOR <= 23 || defined(sun) && PF_MAJOR == 5 && PF_MINOR <= 10
            // Old versions of HPUX and Sun do not have ifr_mtu
            instance.m_mtu = ifr.ifr_ifru.ifru_metric;
#else
            instance.m_mtu = ifr.ifr_mtu;
#endif
            instance.m_knownAttributesMask |= eMTU;
        }
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Attribute SIOCGIFFLAGS");
        if (deps->ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0) {
            instance.m_up = (ifr.ifr_flags & IFF_UP) != 0;
            instance.m_running = (ifr.ifr_flags & IFF_RUNNING) != 0;
            instance.m_knownAttributesMask |= eUp;
            instance.m_knownAttributesMask |= eRunning;
            if (true == instance.m_running) {
                instance.m_availability = eAvailabilityRunningOrFullPower;
                instance.m_netConnectionStatus=eNetConnectionStatusConnected;
            }
            else{
                instance.m_availability = eAvailabilityUnknown;
                if (true == instance.m_up) {
                    instance.m_netConnectionStatus=eNetConnectionStatusMediaDisconnected;
                }
                else {
                    instance.m_netConnectionStatus=eNetConnectionStatusDisconnected;
                }
            }
        }
#if defined(sun) || defined(linux)
        instance.ParseIndex(fd, deps);
#endif

        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll ParseIPv6Addr");
        instance.ParseIPv6Addr(deps);

#if defined(sun)
        instance.ParseMacAddr(fd, deps);
        instance.GetAttributesUsingKstat(deps);
#endif

#if defined(aix)
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Get_NDD_STAT");
        instance.Get_NDD_STAT(deps);
        SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll ParseMacAddrAix");
        instance.ParseMacAddrAix(deps);
#endif

#if defined(linux)
        instance.ParseHwAddr(fd, deps);
        instance.ParseEthtool(fd, deps);
#endif

#if defined(hpux)
        instance.Get_DataLink_Speed(deps);
#endif

    }
    close(fd);

    SCX_LOGTRACE(m_log, L"NetworkInterfaceInfo::FindAll Setting up result list");
    std::vector<NetworkInterfaceInfo> resultList;
    for (size_t nr = 0; nr < interfaces.size(); nr++) {
        NetworkInterfaceInfo &instance = interfaces[nr];

        // If this interface is "UP" or "RUNNING", add it to our valid list (if needed).
        if ((instance.IsKnownIfUp() && instance.IsUp()) ||
            (instance.IsKnownIfRunning() && instance.IsRunning()))
        {
            if (!IsOrWasRunningInterface(instance.GetName()))
            {
                s_validInterfaces.push_back(instance.GetName());
            }
        }

        // Only return the interface if it's in our valid list, unless we are looking for non running interfaces as well.
        if (includeNonRunning || IsOrWasRunningInterface(instance.GetName()))
        {
            resultList.push_back(instance);
        }
    }

    return resultList;
}

/*----------------------------------------------------------------------------*/
//! Construct an instance out of known information
//! \param[in]  name    Name that identifies an interface
//! \param[in]  knownAttributesMask Bitmask where bits set indicates existing optional attriutes
//! \param[in]  ipAddress   IP-address assigned to the interface
//! \param[in]  netmask     Netmask indicating which network the interface belongs to
//! \param[in]  broadcastAddress    Broadcast address used by the interface
//! \param[in]  bytesSent           Number of bytes sent from the interface
//! \param[in]  bytesReceived       Number of bytes received on the interface
//! \param[in]  packetsSent         Number of packets sent from the interface
//! \param[in]  packetsReceived     Number of packets received on the interface
//! \param[in]  errorsSending       Number of errors that occurred when sending from the interface
//! \param[in]  errorsReceiving     Number of errors that occurred when receiving on the interface
//! \param[in]  collisions          Number of collisions that occured in communication
//! \param[in]  up                  Is the the interface "up"
//! \param[in]  running             Is the interface "running"
//! \param[in]  deps                Dependencies to rely on when information is refreshed
NetworkInterfaceInfo::NetworkInterfaceInfo(const wstring &name, unsigned knownAttributesMask,
        const wstring &ipAddress, const wstring &netmask, const wstring &broadcastAddress,
        scxulong bytesSent, scxulong bytesReceived,
        scxulong packetsSent, scxulong packetsReceived,
        scxulong errorsSending, scxulong errorsReceiving,
        scxulong collisions,
        bool up, bool running,
        SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps)
        : m_name(name), m_knownAttributesMask(knownAttributesMask),
          m_ipAddress(ipAddress), m_netmask(netmask), m_broadcastAddress(broadcastAddress),
          m_bytesSent(bytesSent), m_bytesReceived(bytesReceived),
          m_packetsSent(packetsSent), m_packetsReceived(packetsReceived),
          m_errorsSending(errorsSending), m_errorsReceiving(errorsReceiving),
          m_collisions(collisions),
          m_up(up), m_running(running),
          m_deps(deps)
{
    init();
    m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.networkinterface"));
}


/*----------------------------------------------------------------------------*/
//! Private constructor
//! \param[in]  deps    Dependencies to rely on
NetworkInterfaceInfo::NetworkInterfaceInfo(SCXHandle<NetworkInterfaceDependencies> deps) :
    m_knownAttributesMask(0),
    m_bytesSent(0), m_bytesReceived(0),
    m_packetsSent(0), m_packetsReceived(0),
    m_errorsSending(0), m_errorsReceiving(0),
    m_collisions(0), m_up(false), m_running(false),
    m_deps(deps)
{
    init();
    m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.networkinterface"));
}

//! init some private members
//!
void NetworkInterfaceInfo::init()
{
    m_availability        = eAvailabilityInvalid;
    m_adapterTypeID       = eNetworkAdapterTypeInvalid;
    m_autoSense           = false;
    m_interfaceIndex      = 0;
    m_macAddress.clear();
    m_maxSpeed            = 0;
    m_netConnectionStatus = eNetConnectionStatusInvalid;
    m_physicalAdapter     = true;
    m_speed               = 0;
    m_mtu                 = 0;
}
}


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
