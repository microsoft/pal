/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Specification of network interface PAL 
    
    \date        08-03-03 12:23:02
    
*/
/*----------------------------------------------------------------------------*/
#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxassert.h>
#include <scxcorelib/scxlog.h>
#include <scxsystemlib/scxdatadef.h>
#include <algorithm>
#include <vector>
#include <string>
 
#include <sys/socket.h>
#if defined(sun)
#include <unistd.h>
#include <scxsystemlib/scxkstat.h>
#elif defined(aix)
#include <unistd.h>
#include <libperfstat.h>
#include <sys/ndd.h>
#elif defined(hpux)
#include <unistd.h>
#include <sys/stropts.h>
#elif defined(linux)
#include <ifaddrs.h>
#include <unistd.h>
#endif

#if (defined(sun) && defined(sparc) && PF_MINOR==10)
    #define SOLARIS_SPARC_10 
#endif


namespace SCXSystemLib {
    /*----------------------------------------------------------------------------*/
    //! Encapsulates all external dependencies of the PAL
    class NetworkInterfaceDependencies 
    {
    public:
        //! Constructor
        NetworkInterfaceDependencies() { }
                
        /*----------------------------------------------------------------------------*/
        /**
            Creates an endpoint for communication.

            \param domain   - communications domain, selects the protocol family.
            \param type     - socket type, specifies the communication semantics.
            \param protocol - protocol to be used with the socket.
            \returns          on success a file descriptor of new socket, otherwise returns -1 and sets errno.
        */
        virtual int socket(int domain, int type, int protocol)
        {
            return ::socket(domain, type, protocol);
        }
        /*----------------------------------------------------------------------------*/
        /**
            Closes the file descriptor.

            \param fd       - file descriptor to be closed.
            \returns          zero on success, otherwise returns -1 and sets errno.
        */
        virtual int close(int fd)
        {
            return ::close(fd);
        }
#if defined(linux)
        virtual SCXCoreLib::SCXFilePath GetDynamicInfoFile() const;
        /*----------------------------------------------------------------------------*/
        /**
            Creates linked list of structures describing the network interfaces.

            \param ifap     - address of the first item.
            \returns          on success returns 0, otherwise returns -1 and sets errno.
        */
        virtual int getifaddrs(struct ifaddrs **ifap)
        {
            return ::getifaddrs(ifap);
        }
        /*----------------------------------------------------------------------------*/
        /**
            Deletes linked list of structures describing the network interfaces.

            \param ifap     - address of the first item to be deleted, as returned by getifaddrs.
        */
        virtual void freeifaddrs(struct ifaddrs *ifa)
        {
            ::freeifaddrs(ifa);
        }
        /*----------------------------------------------------------------------------*/
        /**
            Sends a message to another socket.

            \param sock     - sending socket file descriptor.
            \param buf      - buffer containing the message to be sent.
            \param          - length of the message to be sent.
            \param flags    - flags.
            \returns          on success returns number of characters sent, otherwise returns -1 and sets errno.
        */
        virtual ssize_t send(int sockfd, const void *buf, size_t len, int flags)
        {
            return ::send(sockfd, buf, len, flags);
        }
        /*----------------------------------------------------------------------------*/
        /**
            Receives a message from a socket.

            \param sock     - receiving socket file descriptor.
            \param buf      - buffer to receive the messages.
            \param          - length of the buffer.
            \param flags    - flags.
            \returns          on success returns number of characters received, otherwise returns -1 and sets errno.
        */
        virtual ssize_t recv(int sockfd, void *buf, size_t len, int flags)
        {
            return ::recv(sockfd, buf, len, flags);
        }
#elif defined(aix)
        virtual int perfstat_netinterface(perfstat_id_t *name, perfstat_netinterface_t *userbuff,
                                          size_t sizeof_struct, int desired_number);
        
        virtual int getkerninfo(int func, char *kinfoStruct, int *totSize, int32long64_t args);

        virtual int bind(int s, const struct sockaddr *name, socklen_t namelen);
#elif defined(sun)
        virtual SCXCoreLib::SCXHandle<SCXKstat> CreateKstat();
#elif defined(hpux)
        virtual int open(const char * path, int oflag);
        virtual int getmsg(int fildes, struct ::strbuf * ctlptr, struct ::strbuf * dataptr, int * flagsp);
        virtual int putmsg(int fildes, const struct ::strbuf * ctlptr, const struct ::strbuf * dataptr, int flags);
#endif
        virtual int ioctl(int fildes, int request, void *ifreqptr);
        /*----------------------------------------------------------------------------*/
        /**
            For testing purposes, determines if the code should rethrow exceptions so they can be caught in the test code.

            \returns true if rethrowing is enabled, false otherwise.
        */
        virtual bool ShouldRethrow()
        {
            return false;
        }
        //! Destructor
        virtual ~NetworkInterfaceDependencies() { }
    protected:
        //! Prevent copying to avoid slicing
        NetworkInterfaceDependencies(const NetworkInterfaceDependencies &);
    };
    /*----------------------------------------------------------------------------*/
    //!
    //! enum for network adapter
    //!
    enum NetworkAdapterTypeIDType
    {
        eNetworkAdapterTypeInvalid                      = 0xFFFF, //!< invalid type id
        eNetworkAdapterTypeEthernet8023                 = 0x0, //!< 0 (0x0) Ethernet 802.3
        eNetworkAdapterTypeTokenRing8025                = 0x1, //!< 1 (0x1) Token Ring 802.5
        eNetworkAdapterTypeFDDI                         = 0x2, //!< 2 (0x2) Fiber Distributed Data Interface (FDDI)
        eNetworkAdapterTypeWAN                          = 0x3, //!< 3 (0x3) Wide Area Network (WAN)
        eNetworkAdapterTypeLocalTalk                    = 0x4, //!< 4 (0x4) LocalTalk
        eNetworkAdapterTypeEthernetUsingDIXHeaderFormat = 0x5, //!< 5 (0x5) Ethernet using DIX header format
        eNetworkAdapterTypeARCNET                       = 0x6, //!< 6 (0x6) ARCNET
        eNetworkAdapterTypeARCNET8782                   = 0x7, //!< 7 (0x7) ARCNET (878.2)
        eNetworkAdapterTypeATM                          = 0x8, //!< 8 (0x8) ATM
        eNetworkAdapterTypeWireless                     = 0x9, //!< 9 (0x9) Wireless 
        eNetworkAdapterTypeInfraredWireless             = 0xa, //!< 10 (0xA) Infrared Wireless
        eNetworkAdapterTypeBpc                          = 0xb, //!< 11 (0xB) Bpc
        eNetworkAdapterTypeCoWan                        = 0xc, //!< 12 (0xC) CoWan
        eNetworkAdapterType1394                         = 0xd, //!< 13 (0xD) 1394
        eNetworkAdapterTypeCnt                                 //!< valid type count
    };
        /*----------------------------------------------------------------------------*/
    //!
    //! enum for network adapter
    //!
    enum NetConnectionStatus
    {
        eNetConnectionStatusInvalid                      = 0xFFFF, //!< invalid type id
        eNetConnectionStatusDisconnected                 = 0x0, //!< 0 (0x0) Disconnected
        eNetConnectionStatusConnecting                   = 0x1, //!< 1 (0x1) Connecting
        eNetConnectionStatusConnected                    = 0x2, //!< 2 (0x2) Connected
        eNetConnectionStatusDisconnecting                = 0x3, //!< 3 (0x3) Disconnecting
        eNetConnectionStatusHardwareNotPresent           = 0x4, //!< 4 (0x4) Hardware not present
        eNetConnectionStatusHardwareDisabled             = 0x5, //!< 5 (0x5) Hardware disabled
        eNetConnectionStatusHardwareMalfunction          = 0x6, //!< 6 (0x6) Media disconnected
        eNetConnectionStatusMediaDisconnected            = 0x7, //!< 7 (0x7) Media disconnected
        eNetConnectionStatusAuthenticating               = 0x8, //!< 8 (0x8) Authenticating
        eNetConnectionStatusAuthenticationSucceeded      = 0x9, //!< 9 (0x9) Authentication succeeded 
        eNetConnectionStatusAuthenticationFailed         = 0xa, //!< 10 (0xA) Authentication failed
        eNetConnectionStatusInvalidAddress               = 0xb, //!< 11 (0xB) Invalid address
        eNetConnectionStatusCredentialsRequired          = 0xc, //!< 12 (0xC) Credentials required
        eNetConnectionStatusCnt                                 //!< valid type count
    };

    /*----------------------------------------------------------------------------*/
    //!
    //! string names for network AdapterType
    //!
    static const std::wstring AdapterTypeNames[eNetworkAdapterTypeCnt] =
    {
        L"Ethernet 802.3",                            //!< 0 (0x0) Ethernet 802.3                         
        L"Token Ring 802.5",                          //!< 1 (0x1) Token Ring 802.5                       
        L"Fiber Distributed Data Interface (FDDI)",   //!< 2 (0x2) Fiber Distributed Data Interface (FDDI)
        L"Wide Area Network (WAN)",                   //!< 3 (0x3) Wide Area Network (WAN)                
        L"LocalTalk",                                 //!< 4 (0x4) LocalTalk                              
        L"Ethernet using DIX header format",          //!< 5 (0x5) Ethernet using DIX header format       
        L"ARCNET",                                    //!< 6 (0x6) ARCNET                                 
        L"ARCNET (878.2)",                            //!< 7 (0x7) ARCNET (878.2)                         
        L"ATM",                                       //!< 8 (0x8) ATM                                    
        L"Wireless",                                  //!< 9 (0x9) Wireless                               
        L"Infrared Wireless",                         //!< 10 (0xA) Infrared Wireless                     
        L"Bpc",                                       //!< 11 (0xB) Bpc                                   
        L"CoWan",                                     //!< 12 (0xC) CoWan                                 
        L"1394"                                       //!< 13 (0xD) 1394
    };
    /*----------------------------------------------------------------------------*/
    //! Information about a network interface
    //! \note IPAddress, netmask and broadcast address are only available if the interface 
    //!       up and running
    class NetworkInterfaceInfo {
    public:
        //! Identifers for attributes whose value might not be known
        enum OptionalAttribute {
            eIPAddress        = 1,         //!< Represents the attribute "IP Address"
            eNetmask          = (1 << 1 ), //!< Represents the attribute "netmask"
            eBroadcastAddress = (1 << 2 ), //!< Represents the attribute "broadcast address"
            eBytesReceived    = (1 << 3 ), //!< Represents the attribute "bytes received"
            eBytesSent        = (1 << 4 ), //!< Represents the attribute "bytes sent"
            ePacketsReceived  = (1 << 5 ), //!< Represents the attribute "packets received"
            ePacketsSent      = (1 << 6 ), //!< Represents the attribute "packets sent"
            eErrorsReceiving  = (1 << 7 ), //!< Represents the attribute "errors receiving"
            eErrorsSending    = (1 << 8 ), //!< Represents the attribute "errors sending"
            eCollisions       = (1 << 9 ), //!< Represents the attribute "collisions"
            eUp               = (1 << 10), //!< Represents the attribute "up"
            eRunning          = (1 << 11), //!< Represents the attribute "running"
            ePhysicalAdapter  = (1 << 12), //!< Represents the attribute "PhysicalAdapter"
            eAutoSense        = (1 << 13), //!< Represents the attribute "AutoSense"
            eInterfaceIndex   = (1 << 14), //!< represents the attribute "InterfaceIndex"
            eSpeed            = (1 << 15), //!< represents the attribute "Speed"
            eMTU              = (1 << 16)  //!< represents the attribute "MTU"
         };

        static std::vector<NetworkInterfaceInfo> FindAll(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps,
                                                         bool includeNonRunning = false);

        /* The speed values: 10Mb, 100Mb, gigabit, 10Gb. */
        enum 
        {
            SPEED_10    = 10000,
            SPEED_100   = 100000,
            SPEED_1000  = 1000000,
            SPEED_10000 = 10000000
        };

        NetworkInterfaceInfo(const std::wstring &name, unsigned knownAttributesMask, 
                const std::wstring &ipAddress, const std::wstring &netmask, const std::wstring &broadcastAddress,
                scxulong bytesSent, scxulong bytesReceived,
                scxulong packetsSent, scxulong packetsReceived,
                scxulong errorsSending, scxulong errorsReceiving,
                scxulong collisions,
                bool up, bool running,
                SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

        void Refresh();
        
        //! Name of interface
        //! \returns Name
        std::wstring GetName() const {return m_name; }
        
        //! Check if the value of an attribute is known
        //! \param[in] attr  Attribbute of interest
        //! \returns true iff the value of the optional attribute is present
        bool IsValueKnown(OptionalAttribute attr) const { return (m_knownAttributesMask & attr) != 0; }
        
        //! Check if the IPAddress is known
        //! \returns true iff the ipaddress is known
        bool IsIPAddressKnown() const {return IsValueKnown(eIPAddress); }
        
        //! IPv4 Address assigned to interface
        //! \returns IPv4 Address
        std::wstring GetIPAddress() const {SCXASSERT(IsValueKnown(eIPAddress)); return m_ipAddress; }
                                    
        //! IPv6 Address assigned to interface
        //! \returns IPv6 Address
        std::vector<std::wstring> GetIPV6Address() const {return m_ipv6Address; }

        //! Check if the netmask is known
        //! \returns true iff the netmask is known
        bool IsNetmaskKnown() const {return IsValueKnown(eNetmask); }

        //! Netmask assigned to interface
        //! \returns Netmask
        std::wstring GetNetmask() const {SCXASSERT(IsValueKnown(eNetmask)); return m_netmask; }
        
        //! Check if the broadcast address is known
        //! \returns true iff the broadcast address is known
        bool IsBroadcastAddressKnown() const {return IsValueKnown(eBroadcastAddress); }

        //! Broadcast address assigned to interface
        //! \returns Broadcast address
        std::wstring GetBroadcastAddress() const {SCXASSERT(IsValueKnown(eBroadcastAddress)); return m_broadcastAddress; }
        
        //! Check if bytes received is known
        //! \returns true iff bytes received is known
        bool IsBytesReceivedKnown() const {return IsValueKnown(eBytesReceived); }

        //! Number of bytes received from interface
        //! \returns    Number of bytes
        scxulong GetBytesReceived() const {SCXASSERT(IsValueKnown(eBytesReceived)); return m_bytesReceived; }
        
        //! Check if bytes sent is known
        //! \returns true iff bytes sent is known
        bool IsBytesSentKnown() const {return IsValueKnown(eBytesSent); }

        //! Number of bytes sent to interface
        //! \returns    Number of bytes
        scxulong GetBytesSent() const {SCXASSERT(IsValueKnown(eBytesSent)); return m_bytesSent; }
                
        //! Check if packets received is known
        //! \returns true iff packets received is known
        bool IsPacketsReceivedKnown() const {return IsValueKnown(ePacketsReceived); }

        //! Number of packets received from interface
        //! \returns    Number of packets
        scxulong GetPacketsReceived() const {SCXASSERT(IsValueKnown(ePacketsReceived)); return m_packetsReceived; }
        
        //! Check if packets sent is known
        //! \returns true iff packets sent is known
        bool IsPacketsSentKnown() const {return IsValueKnown(ePacketsSent); }

        //! Number of packets sent to interface
        //! \returns    Number of packets
        scxulong GetPacketsSent() const {SCXASSERT(IsValueKnown(ePacketsSent)); return m_packetsSent; }
        
        //! Check if receive errors is known
        //! \returns true iff receive errors is known
        bool IsKnownIfReceiveErrors() const {return IsValueKnown(eErrorsReceiving); }

        //! Number of errors that have occurred when receiving from interface
        //! \returns    Number of errors
        scxulong GetErrorsReceiving() const {SCXASSERT(IsKnownIfReceiveErrors()); return m_errorsReceiving; }        
        
        //! Check if send errors is known
        //! \returns true iff send errors is known
        bool IsKnownIfSendErrors() const {return IsValueKnown(eErrorsSending); }

        //! Number of errors that have occurred when sending to interface
        //! \returns    Number of errors
        scxulong GetErrorsSending() const {SCXASSERT(IsKnownIfSendErrors()); return m_errorsSending; }
        
        //! Check if collisions is known
        //! \returns true iff collisions is known
        bool IsKnownIfCollisions() const {return IsValueKnown(eCollisions); }
        
        //! Number of collisons that have occurred on interface
        //! \returns Number of collisions
        scxulong GetCollisions() const {SCXASSERT(IsKnownIfCollisions()); return m_collisions; }
        
        //! Check if up is known
        //! \returns true iff up is known
        bool IsKnownIfUp() const {return IsValueKnown(eUp); }

        //! Is the interface up
        //! \returns true iff the interface is up
        bool IsUp() const {SCXASSERT(IsValueKnown(eUp)); return m_up; }
        
        //! Check if running is known
        //! \returns true iff running is known
        bool IsKnownIfRunning() const {return IsValueKnown(eRunning); }

        //! Is the interface running, that is, are resources allocated
        //! \returns true iff the interface is running
        bool IsRunning() const {SCXASSERT(IsValueKnown(eRunning)); return m_running; }
        
        /*----------------------------------------------------------------------------*/
        /**
           Retrieve availability and status of the device.

           \param       value - output parameter where the availability of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetAvailability(unsigned short& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve network medium in use of the device.

           \param       value - output parameter where the AdapterType of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetAdapterType(std::wstring& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve network medium ID in use of the device.

           \param       value - output parameter where the AdapterTypeID of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetAdapterTypeID(unsigned short& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve if the network adapter can automatically determine the speed of the attached or network media.

           \param       value - output parameter where the AutoSense of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetAutoSense(bool& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve index value that uniquely identifies the local network interface of the device.

           \param       value - output parameter where the InterfaceIndex of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetInterfaceIndex(unsigned int& value) const;

        /*----------------------------------------------------------------------------*/
        /**
          Retrieve the raw form of the media access control address for this network 
          adapter of the device where the case is preserved and there is no delimiter.

          \param value - output parameter where the MACAddress of the device is stored.
          \returns true if value is supported on platform, false otherwise.
        */
        bool GetMACAddressRAW(std::wstring& value) const;

        /*----------------------------------------------------------------------------*/
        /**
          Retrieve the formatted form of media access control address for this network 
          adapter of the device.

          \param value - output parameter where the MACAddress of the device is stored.
          \param sepChar - the delimiter.
          \param upperCase - true for uppercase and false to preserve the case.
          \returns true if value is supported on platform, false otherwise.
        */
        bool GetMACAddress(std::wstring& value, const char sepChar =':',
                           bool upperCase=false) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve maximum speed, in bits per second, for the network adapter.

           \param       value - output parameter where the MaxSpeed of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetMaxSpeed(scxulong& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve state of the network adapter connection to the network.

           \param       value - output parameter where the NetConnectionStatus of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetNetConnectionStatus(unsigned short& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve whether the adapter is a physical or a logical adapter.

           \param       value - output parameter where the PhysicalAdapter of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetPhysicalAdapter(bool& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve estimate of the current bandwidth in bits per second.

           \param       value - output parameter where the Speed of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetSpeed(scxulong& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the maximum transmission unit (MTU).

           \param       value - output parameter where the MTU of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetMTU(scxulong& value) const;
        
        const std::wstring DumpString() const;        


        //! Clear the list of running interfaces
        //! (This is only used within test code - we need some refactoring to eliminate this)
        static void ClearRunningInterfaceList() { s_validInterfaces.clear(); }

        SCXCoreLib::SCXLogHandle m_log; //! Handle to log file 

    private: 
#if defined(aix)
        static void FindAllUsingPerfStat(std::vector<NetworkInterfaceInfo> &interfaces,
                                         SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif
#if defined(hpux)
        static void FindAllInDLPI(std::vector<NetworkInterfaceInfo> &interfaces,
                                  SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps); 
#endif

#if defined(sun)
        static void FindAllUsingKStat(std::vector<NetworkInterfaceInfo> &interfaces,
                                      SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

        /*----------------------------------------------------------------------------*/
        /**
           Parse Mac Addr using  arp.
            \param fd file descriptor
            \param deps dependency
        */
        void ParseMacAddr(int fd, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

        /*----------------------------------------------------------------------------*/
        /**
           Get Attributes Using Kstat
           \param       deps - dependency
        */
        void GetAttributesUsingKstat(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif

#if defined(linux) 
        static void FindAllInFile(std::vector<NetworkInterfaceInfo> &interfaces,
                                  SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif

#if defined(aix)
        /*----------------------------------------------------------------------------*/
        /**
           Parse Mac Addr using getkerninfo to set the NetworkAdapterType 
              and NetworkAdapterTypeID.
            \param instName: Interface Name
        */
        void ParseMacAddrAix(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif

#if defined(linux)
        //! parase data get by ioctl(fd, SIOCGIFHWADDR,), this function will get
        //! AdapterTypeID, AdapterType, PhysicalAdapter, MACAddress.
        //!
        //! \param fd file descriptor
        //! \param deps dependency
        //!
        void ParseHwAddr(int fd, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
        //! parse data get by ioctl(fd, SIOETHTOOL, ), will get attribute
        //! AutoSense, MaxSpeed, Speed
        //!
        //! \param fd file descriptor
        //! \param deps dependency
        //!
        void ParseEthtool(int fd, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif

#if defined(hpux)
        //! Description: This function retrieves the speed of the underlying 
        //! link in Mbits/sec. This function also detects autonegotiate option.
        //! Reference: http://h10032.www1.hp.com/ctg/Manual/c02011471.pdf
        //!             HP DLPI Programmer's Guide, HP-UX 11i v3
        //!             Manufacturing Part Number: 5991-7498(Februray 2007)
        //!                  Chapter 3 and Appendix A 
        //!
        //! \param Inputs: deps - dependency for the Network Interface Instance.
        //!
        //! \param Output:  m_speed, m_autoSense, m_knownAttributesMask.
        //!
        //! \param Return: none. 
        //!
        void Get_DataLink_Speed(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif

#if defined(aix)

        /**
           Helper class to insure that a file descriptor is closed properly regardless of exceptions
        */
        class AutoClose 
        {
            public:
                /*
                   AutoClose constructor
                   \param[in]    fd      File descriptor to close
                   */
                AutoClose(SCXCoreLib::SCXLogHandle log, int fd) : m_log(log), m_fd(fd) {}
                ~AutoClose();
                SCXCoreLib::SCXLogHandle m_log;     //!< Log handle.
                int m_fd;       //!< File descriptor
        }; // AutoClose class.

        // Helper function for Get_NDD_STAT() function.
        void set_speed(const scxulong speed_selected, const scxulong auto_speed);

        //! Description: This function retrieves the selected speed of the 
        //! ethernet adapter as well as the max speed of the adapter in 
        //! question. It also detects if the ether adapter is set for
        //! autonegotiation.
        //! (Ref: http://publib.boulder.ibm.com/infocenter/pseries/v5r3/index.jsp?topic=/com.ibm.aix.kernelext/doc/kernextc/ethernet_dd.htm
        //!                       AIX 5L version 5.3
        //!        Kernel Extensions and Device Dupport Programming Concepts: Ethernet Device Drivers
        //!        SC23-4900-06, Seventh Edition(October 2009)
        //!
        //! \param Inputs: deps - dependency for the Network Interface 
        //!                       Instance.
        //!
        //! \param Output: m_maxSpeed, m_speed, m_autoSense, 
        //!                m_knownAttributesMask.
        //!
        //! \param Return: none. 
        //!
        void Get_NDD_STAT(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

#endif // defined(aix)


#if defined(sun) || defined(linux)
        //! parse data get by ioctl(fd, SIOCGIFINDEX, ), will get InterfaceIndex.The linux and solaris share the same method
        //!
        //! \param fd file descriptor
        //! \param deps dependency
        //! 
        void ParseIndex(int fd, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
#endif
        //! Parse IPv6 addresses
        //! \param[in]  deps    What this PAL depends on.
        void ParseIPv6Addr(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

        //! init some private members
        //!
        void init();

        NetworkInterfaceInfo(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);

        //! Test if this interface name is either currently running or was ever
        //! running in the past.  If the interface was never running, it's not
        //! returned.
        //! \returns true iff Interface is or has been running in the past
        static bool IsOrWasRunningInterface(std::wstring name)
        {
            return (s_validInterfaces.end() !=
                    std::find(s_validInterfaces.begin(), s_validInterfaces.end(), name));
        }

        std::wstring m_name;         //!< Name of interface
        unsigned long m_knownAttributesMask;   //!< Bitmask holding which attributes that have known values 
        std::wstring m_ipAddress;         //!< IPv4 Address (empty if none is available)
        std::vector <std::wstring> m_ipv6Address; //!< IPv6 Address (empty if none is available)
        std::wstring m_netmask;          //!< Netmask (empty if none is available)
        std::wstring m_broadcastAddress;  //!< Broadcast address (empty if none is available)
        scxulong m_bytesSent;        //!< Number of bytes sent to interface
        scxulong m_bytesReceived;    //!< Number of bytes received from interface
        scxulong m_packetsSent;      //!< Number of packets sent to interface
        scxulong m_packetsReceived;  //!< Number of bytes received from interface
        scxulong m_errorsSending;    //!< Number of errors that have occurred when sending to interface
        scxulong m_errorsReceiving;  //!< Number of errors that have occurred when receiving from interface
        scxulong m_collisions;       //!< Number of collisons that have occurred on interface
        bool m_up;                   //!< Is the interface up
        bool m_running;              //!< Is the interface running

        unsigned short m_availability;  //!< Availability and status of the device, must be value in enum DiskAvailabilityType
        std::wstring m_adapterType;     //!< Network medium in use
        unsigned short m_adapterTypeID; //!< Network medium ID in use
        bool m_autoSense;               //!< if the network adapter can automatically determine the speed of the attached or network media
        unsigned int  m_interfaceIndex; //!< Index value that uniquely identifies the local network interface
        std::wstring m_macAddress;      //!< Media access control address for this network adapter
        scxulong m_maxSpeed;            //!< Maximum speed, in bits per second, for the network adapter
        unsigned short m_netConnectionStatus; //!< State of the network adapter connection to the network
        bool m_physicalAdapter;         //!< Indicates whether the adapter is a physical or a logical adapter
        scxulong m_speed;               //!< Estimate of the current bandwidth in bits per second
        scxulong m_mtu;                 //!< Maximum transmission unit

#if defined(sun) 
        std::wstring m_ks_module;   //!< save the ks_module of this instance in kstat
        int       m_ks_instance; //!< save the ks_instance of this instance in kstat
#endif

#if defined(aix)
        typedef std::map<unsigned int, NetworkAdapterTypeIDType> NDDMAP;
        typedef NDDMAP::value_type NDDMAPPair;

        static const NDDMAP      m_mapNWAdaptTypeIdToNDDType;
        static const NDDMAPPair  m_netwTypePairs[];
#endif

        SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> m_deps; //!< Dependencies to rely on
        static std::vector<std::wstring> s_validInterfaces; //!< List of interfaces that have been running

    };
}


#endif /* NETWORKINTERFACE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
