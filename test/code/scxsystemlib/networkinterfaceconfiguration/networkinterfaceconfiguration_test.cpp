/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Test of network interface configuration

    \date        2012-05-022 16:03:56

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/networkinterfaceconfigurationenumeration.h>
#include <scxsystemlib/networkinterface.h>
#include <scxsystemlib/processenumeration.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxfile.h>
#include <testutils/scxtestutils.h> // For SelfDeletingFilePath
#include <scxcorelib/scxnameresolver.h>
#include <errno.h>
#if defined(linux)
#if PF_MAJOR <=4
#include <linux/types.h>
#endif
#include <linux/rtnetlink.h>
#endif
#include <arpa/inet.h>
#include <scxsystemlib/scxnetworkadapterip_test.h>

using namespace SCXSystemLib;
using namespace SCXCoreLib;
using namespace std;

class TestProcessInstance : public ProcessInstance
{
public:
    TestProcessInstance(const std::string &cmd, const std::string &params) : ProcessInstance(cmd, params)
    {
    }
};

class TestNetworkInstanceConfigurationEnumerationDeps : public NetworkInstanceConfigurationEnumerationDeps 
{
public:
    // ! Constructor
    TestNetworkInstanceConfigurationEnumerationDeps(const std::string &cmd1, const std::string &param1,
                                                    const std::string &cmd2, const std::string &param2) 
    { 
        m_commands.clear();
        m_commands.push_back(cmd1);
        m_commands.push_back(param1);
        m_commands.push_back(cmd2);
        m_commands.push_back(param2);
    }

    /**
    * Overriding ProcessEnumeration::Find() for unit-testing.
    \param[in] name  Name of process that we're searching for
    \returns A vector with process instance pointer.
    */
    virtual std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > Find(const std::wstring& name, ProcessEnumeration& procEnum)
    {
        (void)name;
        (void)procEnum;
        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > proc_v;
        unsigned int i=0;
        while (i < m_commands.size() && m_commands.size()>2)
        {
            SCXCoreLib::SCXHandle<TestProcessInstance> inst(new TestProcessInstance(m_commands[i], m_commands[i+1]));
            proc_v.push_back(inst);
            i+=2;
        }
        return proc_v;
    }

    // ! Destructor
    virtual ~TestNetworkInstanceConfigurationEnumerationDeps () { }
private:
    std::vector<std::string> m_commands;
};
               
// For debugging purposes set s_InstrumentTests to true. 
const static bool s_InstrumentTests = false;

// Test dependencies.

// This dependency class does not override any system calls but just forces the provider to pass exceptions to the
// test code so they can be properly reported.
class NetworkInterfaceDependenciesRethrow: public NetworkInterfaceDependencies
{
    /*----------------------------------------------------------------------------*/
    /**
        Forces the production code to pass the thrown exceptions to the test code.

        \returns true to force production code to rethrow the getaway IP exception to the test code.
    */
    virtual bool ShouldRethrow()
    {
        if(s_InstrumentTests)
        {
            cout<<"shouldRethrow()"<<endl;
        }
        return m_shouldRethrow;
    }
    bool m_shouldRethrow;
public:
    /*----------------------------------------------------------------------------*/
    /**
        Constructor.
    */
    NetworkInterfaceDependenciesRethrow(): m_shouldRethrow(true)
    {
    }
    /*----------------------------------------------------------------------------*/
    /**
        Turns on or off exception rethrowing.

        \param shouldRethrow - if true provider rethrows caugth exception and passes it to the test code.
    */
    void EnableRethrow(bool shouldRethrow = true)
    {
        m_shouldRethrow = shouldRethrow;
    }
};

#if defined(linux)

// This dependency class overrides system calls necessary for default gateway detection and creates mock default gateway
// address.
class NetworkInterfaceDependenciesDefGatewayIP: public NetworkInterfaceDependenciesRethrow
{
private:
    /*----------------------------------------------------------------------------*/
    /**
        Returns default gateway address.

        \returns default gateway address.
    */
    in_addr_t DefaultGatewayAddress()
    {
        return 66666666;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Returns file descriptor of the opened socket. Tests support only one opened socket at a time.
        High value number is used so in the case that some of the functions that are not overriden in the
        test dependency try to call the OS with the given file descriptor, they would fail.

        \returns file descriptor of the opened socket.
    */
    int OpenedSocketFD()
    {
        return 5555;
    }
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
        if(s_InstrumentTests)
        {
            cout<<"socket() = "<<OpenedSocketFD()<<endl;
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Invalid parameter domain whan calling socket(), only PF_NETLINK is supported",
                PF_NETLINK, domain);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Invalid parameter type whan calling socket(), only SOCK_DGRAMK is supported",
                SOCK_DGRAM, type);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Invalid parameter protocol whan calling socket(), only NETLINK_ROUTE is supported",
                NETLINK_ROUTE, protocol);

        m_recvMsgQueue = -1;// Socket opened, but no recv messages in the queue yet.

        return OpenedSocketFD();
    }
    /*----------------------------------------------------------------------------*/
    /**
        Closes the file descriptor.

        \param fd       - file descriptor to be closed.
        \returns          zero on success, otherwise returns -1 and sets errno.
    */
    virtual int close(int fd)
    {
        if(s_InstrumentTests)
        {
            cout<<"close("<<fd<<")"<<endl;
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Trying to close socket by using wrong file descriptor.", OpenedSocketFD(), fd);
        return 0;
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
    virtual ssize_t send(int sockfd, const void *buf, size_t len, int /*flags*/)
    {
        if(s_InstrumentTests)
        {
            cout<<"send("<<sockfd<<"); m_forceDefGatewayIPFailure = "<<m_forceDefGatewayIPFailure<<endl;
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Trying to send socket message by using wrong file descriptor.",
                OpenedSocketFD(), sockfd);
        CPPUNIT_ASSERT_MESSAGE("Invalid buff when calling send().", buf != NULL);
        CPPUNIT_ASSERT_MESSAGE("Invalid len when calling send().", len >= NLMSG_LENGTH(sizeof(struct rtmsg)));
        const struct nlmsghdr* sendMessageHeader = static_cast<const struct nlmsghdr*>(buf);        
        CPPUNIT_ASSERT_EQUAL(NLMSG_LENGTH(sizeof(struct rtmsg)), sendMessageHeader->nlmsg_len);
        CPPUNIT_ASSERT_EQUAL(static_cast<__u16>(RTM_GETROUTE), sendMessageHeader->nlmsg_type);
        CPPUNIT_ASSERT(sendMessageHeader->nlmsg_flags & NLM_F_REQUEST);

        CPPUNIT_ASSERT_EQUAL_MESSAGE("Sending to request to get network data messages but it was already done.",
                -1, m_recvMsgQueue);

        m_recvMsgQueue = 0;// We have recv messages in the queue ready.
        m_msgSeq = sendMessageHeader->nlmsg_seq;

        return sendMessageHeader->nlmsg_len;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Appends NLMSG_ERROR message to the queue. Used to force the default gateway IP exception for testing purposes.

        \param msgGroup  - Message queue.
        \param alignSize - Padding that needs to be added after this message if one more message is to be added.
    */
    void Append_NLMSG_ERROR(vector<unsigned char> &msgGroup, size_t &alignSize)
    {
        // Size of the entire NL message.
        size_t nlMsgSize = NLMSG_LENGTH(sizeof(nlmsgerr));
        // Extra padding that may be required if another message is appended after this one.
        alignSize = NLMSG_SPACE(sizeof(nlmsgerr)) - nlMsgSize;

        size_t msgPos = msgGroup.size();
        // Add space for this message.
        msgGroup.resize(msgGroup.size() + nlMsgSize, 0);
        
        // Setup NL message header.
        struct nlmsghdr *recvMessage;
        recvMessage = (struct nlmsghdr *)&msgGroup[msgPos];
        recvMessage->nlmsg_len = static_cast<__u32>(nlMsgSize);
        recvMessage->nlmsg_type = NLMSG_ERROR;
        recvMessage->nlmsg_seq = m_msgSeq;

        // Setup the error code.
        struct nlmsgerr *errorMessage = static_cast<struct nlmsgerr *>(NLMSG_DATA(recvMessage));
        errorMessage->error = 6666;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Appends NLMSG_DONE message to the queue.

        \param msgGroup  - Message queue.
        \param alignSize - Padding that needs to be added after this message if one more message is to be added.
    */
    void Append_NLMSG_DONE(vector<unsigned char> &msgGroup, size_t &alignSize)
    {
        // Size of the entire NL message.
        size_t nlMsgSize = NLMSG_LENGTH(0);
        // Extra padding that may be required if another message is appended after this one.
        alignSize = NLMSG_SPACE(0) - nlMsgSize;

        size_t msgPos = msgGroup.size();
        // Add space for this message.
        msgGroup.resize(msgGroup.size() + nlMsgSize, 0);
        
        // Setup NL message header.
        struct nlmsghdr *recvMessage;
        recvMessage = (struct nlmsghdr *)&msgGroup[msgPos];
        recvMessage->nlmsg_len = static_cast<__u32>(nlMsgSize);
        recvMessage->nlmsg_type = NLMSG_DONE;
        recvMessage->nlmsg_seq = m_msgSeq;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Appends RTM_NEWROUTE message to the queue.

        \param msgGroup    - Message queue.
        \param alignSize   - Padding that needs to be added after this message if one more message is to be added.
        \param destAddr    - Destination address attribute.
        \param gatewayAddr - Gateway address attribute.
    */
    void AppendRTMsgWith2Attributes(vector<unsigned char> &msgGroup, size_t &alignSize,
            in_addr_t destAddr, in_addr_t gatewayAddr)
    {
        // Size of the RT part of the message.
        size_t rtMsgSize = NLMSG_ALIGN(sizeof(struct rtmsg)) +
                RTA_SPACE(sizeof(in_addr_t)) + RTA_LENGTH(sizeof(in_addr_t));
        // Size of the entire NL message.
        size_t nlMsgSize = NLMSG_LENGTH(rtMsgSize);
        // Extra padding that may be required if another message is appended after this one.
        alignSize = NLMSG_SPACE(rtMsgSize) - nlMsgSize;

        size_t msgPos = msgGroup.size();
        // Add space for this message.
        msgGroup.resize(msgGroup.size() + nlMsgSize, 0);
        
        // Setup NL message header.
        struct nlmsghdr *recvMessage;
        recvMessage = (struct nlmsghdr *)&msgGroup[msgPos];
        recvMessage->nlmsg_len = static_cast<__u32>(nlMsgSize);
        recvMessage->nlmsg_type = RTM_NEWROUTE;
        recvMessage->nlmsg_seq = m_msgSeq;
        
        // Setup RT message header.
        struct rtmsg *routeMessage = static_cast<struct rtmsg *>(NLMSG_DATA(recvMessage));
        routeMessage->rtm_table = RT_TABLE_MAIN;
        routeMessage->rtm_family = AF_INET;

        // Prepare attribute pointer.
        struct rtattr *routeAttribute = RTM_RTA(routeMessage);
        ssize_t routeAttributeLen = RTM_PAYLOAD(recvMessage);

        // First attribute, destination address.
        routeAttribute->rta_len = RTA_LENGTH(sizeof(in_addr_t));
        routeAttribute->rta_type = RTA_DST;
        *static_cast<in_addr_t *>(RTA_DATA(routeAttribute)) = destAddr;
        
        routeAttribute = RTA_NEXT(routeAttribute, routeAttributeLen);
        
        // Second attribute, gateway address.
        routeAttribute->rta_len = RTA_LENGTH(sizeof(in_addr_t));
        routeAttribute->rta_type = RTA_GATEWAY;
        *static_cast<in_addr_t *>(RTA_DATA(routeAttribute)) = gatewayAddr;
    }
    /*----------------------------------------------------------------------------*/
    /**
        Receives a message from a socket. We simulate a queue containing two groups of network messages
        requiring two calls to the recv() to get all the messages.

        \param sock     - receiving socket file descriptor.
        \param buf      - buffer to receive the messages.
        \param          - length of the buffer.
        \param flags    - flags.
        \returns          on success returns number of characters received, otherwise returns -1 and sets errno.
    */
    virtual ssize_t recv(int sockfd, void *buf, size_t len, int flags)
    {
        if(s_InstrumentTests)
        {
            cout<<"recv("<<sockfd<<","<<len<<")"<<"; m_recvMsgQueue = "<<m_recvMsgQueue<<endl;
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Trying to receive socket message by using wrong file descriptor.",
                OpenedSocketFD(), sockfd);
        CPPUNIT_ASSERT_MESSAGE("Invalid buff when calling recv().", buf != NULL);

        CPPUNIT_ASSERT_MESSAGE("trying to receive network data message but there are no messages in the queue.",
                -1 != m_recvMsgQueue);
        
        vector<unsigned char> msgGroup;
        size_t alignSize = 0;
        switch (m_recvMsgQueue)
        {
        case 0:
            // First call to the recv. Prepare first group of messages, around 1/2 kB of messages, but not our gateweay IP
            // message so there will be next call to the recv().
            {
                while(msgGroup.size() <= 512)
                {
                    // Add alignment padding.
                    msgGroup.resize(msgGroup.size() + alignSize, 0);
                    // Add message.
                    AppendRTMsgWith2Attributes(msgGroup, alignSize, 55555555, DefaultGatewayAddress());
                }
                
                if (m_forceDefGatewayIPFailure)
                {
                    // To force the exception add NLMSG_ERROR message to the queue.
                    // Add alignment padding.
                    msgGroup.resize(msgGroup.size() + alignSize, 0);
                    // Add message.
                    Append_NLMSG_ERROR(msgGroup, alignSize);
                }
                
                break;
            }        
        case 1:
            // Second call to the recv. Add 2k of messages and then our gateweay IP message and finally NLMSG_DONE message.
            {
                while(msgGroup.size() <= 2048)
                {
                    // Add alignment padding.
                    msgGroup.resize(msgGroup.size() + alignSize, 0);
                    // Add message.
                    AppendRTMsgWith2Attributes(msgGroup, alignSize, 55555555, DefaultGatewayAddress());
                }

                // Add alignment padding.
                msgGroup.resize(msgGroup.size() + alignSize, 0);
                // Add gateway IP (destination address must be 0).
                AppendRTMsgWith2Attributes(msgGroup, alignSize, 0, DefaultGatewayAddress());
                
                // Add alignment padding.
                msgGroup.resize(msgGroup.size() + alignSize, 0);
                // Final NLMSG_DONE.
                Append_NLMSG_DONE(msgGroup, alignSize);

                break;
            }
        default:
            CPPUNIT_FAIL("Called recv() too many times, queue already empty.");
        }
        
        // Return data.
        size_t copySize = min(msgGroup.size(), len);
        memcpy(buf, &msgGroup[0], copySize);
        if (!(flags & MSG_PEEK))
        {
            m_recvMsgQueue++;
        }
        return copySize;
    }
    
    int m_forceDefGatewayIPFailure;// Forces default geteway IP code to fail.
    int m_socketFd;// Fd of the open socket or -1 if the socket is closed. Tests support only one socket at a time.
    int m_recvMsgQueue;// recv() queue message group index. One group of messages per call, -1 means no messages
                       // in the queue.
    __u32 m_msgSeq;// Message sequence number that was requested.
public:
    NetworkInterfaceDependenciesDefGatewayIP():
            m_forceDefGatewayIPFailure(false), m_socketFd(-1), m_recvMsgQueue(-1), m_msgSeq(0)
    {
    }
    /*----------------------------------------------------------------------------*/
    /**
        Returns default gateway address string.

        \returns default gateway address string.
    */
    string DefaultGatewayAddressStr()
    {
        in_addr gatewayAddress;
        gatewayAddress.s_addr = DefaultGatewayAddress();
        return string(inet_ntoa(gatewayAddress));
    }
    /*----------------------------------------------------------------------------*/
    /**
        Forces default geteway IP code to fail.

        \param forceDefGatewayIPFailure - if true forces provider to fail in default gateway IP code.
    */
    void ForceDefGatewayIPFailure(bool forceDefGatewayIPFailure = true)
    {
        m_forceDefGatewayIPFailure = forceDefGatewayIPFailure;
    }
};

#endif// defined(linux)

// Tests the network interface PAL
class SCXNetworkInterfaceConfigurationTest : public CPPUNIT_NS::TestFixture /* CUSTOMIZE: use a class name with relevant name */
{
public:
    std::vector<wstring> lines;
    SCXFilePath testFile1;
    SCXFilePath testFile2;
    CPPUNIT_TEST_SUITE( SCXNetworkInterfaceConfigurationTest ); /* CUSTOMIZE: Name must be same as classname */
    CPPUNIT_TEST( TestGetDHCPEnabledFromConfigData );
    CPPUNIT_TEST( TestGetDHCPEnabledFromProcessList );
    CPPUNIT_TEST( TestNetInterfaceConfEnumerationFindAll );
    CPPUNIT_TEST( TestDefaultGatewayIPAddress );
    CPPUNIT_TEST( TestNetInterfaceIPAddress);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp()
    {
        /* CUSTOMIZE: This method will be called once before each test function. Use it to set up commonly used objects */
        wstring fileData = L"";
#if defined(PF_DISTRO_REDHAT)
        fileData = 
            L"DEVICE=\"lan0\"\n"
            L"BOOTPROTO=\"dhcp\"\n"
            L"HWADDR=\"00:21:5E:DB:AC:98\"\n"
            L"ONBOOT=\"yes\"\n";
#elif defined(hpux)
        fileData = 
            L"HOSTNAME=\"scxhpv3-42\"\n"
            L"OPERATING_SYSTEM=\"HP-UX\"\n"
            L"LOOPBACK_ADDRESS=\"127.0.0.1\"\n"
            L"INTERFACE_NAME[0]=lan0\n"
            L"IP_ADDRESS[0]=\"10.217.5.127\"\n"
            L"SUBNET_MASK[0]=\"255.255.254.0\"\n"
            L"BROADCAST_ADDRESS[0]=\"\"\n"
            L"INTERFACE_STATE[0]=\"\"\n"
            L"DHCP_ENABLE[0]=1\n"
            L"INTERFACE_MODULES[0]=\"\"\n"
            L"ROUTE_DESTINATION[0]=\"default\"\n"
            L"ROUTE_MASK[0]=\"\"\n"
            L"ROUTE_GATEWAY[0]=\"10.217.2.1\"\n"
            L"ROUTE_COUNT[0]=\"1\"\n"
            L"ROUTE_ARGS[0]=\"\"\n"
            L"ROUTE_SOURCE[0]=\"\"\n"
            L"GATED=\"0\"\n"
            L"GATED_ARGS=\"\"\n"
            L"RDPD=\"0\"\n"
            L"RARPD=\"0\"\n"
            L"DEFAULT_INTERFACE_MODULES=\"\"\n"
            L"LANCONFIG_ARGS[0]=\"ether\"\n";
#elif defined(sun)
        fileData = L"whatever";
#endif
        testFile1 = SCXFile::CreateTempFile(fileData);
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLines(testFile1.Get(), lines, nlfs);
    }

    void tearDown()
    {
        SCXCoreLib::SelfDeletingFilePath sdel1(testFile1);
        SCXCoreLib::SelfDeletingFilePath sdel2(testFile2);
    }

    void TestGetDHCPEnabledFromProcessList()
    {
        // (WI 516119)
        std::wstring myParams(L"eth0"); // Mock dhcpcd daemon parameters' list
        SCXCoreLib::SCXHandle<TestNetworkInstanceConfigurationEnumerationDeps> deps(new TestNetworkInstanceConfigurationEnumerationDeps("dhcpdc", "eth0", "cmd2", "param2"));
        CPPUNIT_ASSERT
        (
            NetworkInterfaceConfigurationEnumeration::GetDHCPEnabledFromProcessList(myParams, deps) 
        );
        myParams = L"wrongParams";
        CPPUNIT_ASSERT
        (
            !NetworkInterfaceConfigurationEnumeration::GetDHCPEnabledFromProcessList(myParams, deps) 
        );
    }
    
    void TestGetDHCPEnabledFromConfigData()
    {
// AIX and SUSE use output of Process provider methods
#if !defined(aix) && !defined(PF_DISTRO_SUSE) && !defined(PF_DISTRO_ULINUX)
        bool actualResult;
        actualResult= NetworkInterfaceConfigurationEnumeration::GetDHCPEnabledFromConfigData
        (
            lines,
            std::wstring(L"lan0")
        );
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Got wrong value for GetDHCPEnabledFromConfigData", true, actualResult);
        
    #if defined(hpux)
        wstring fileData = 
                      L"INTERFACE_NAME[1]=lan666\n"
                      L"DHCP_ENABLE[1]=0\n"
                      L"ROUTE_DESTINATION[1]=\"default\"\n"
                      L"ROUTE_GATEWAY[1]=\"10.217.2.1\"\n"
                      L"ROUTE_COUNT[1]=1\n"
                      L"ROUTE_DESTINATION[2]=\"default\"\n"
                      L"ROUTE_GATEWAY[2]=\"10.217.4.1\"\n"
                      L"ROUTE_COUNT[2]=\"1\"\n";

        testFile2 = SCXFile::CreateTempFile(fileData);
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLines(testFile2.Get(), lines, nlfs);
        actualResult= NetworkInterfaceConfigurationEnumeration::GetDHCPEnabledFromConfigData
        (
            lines,
            std::wstring(L"elan")
        );
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Got wrong value for GetDHCPEnabledFromConfigData", false, actualResult);
        
        actualResult= NetworkInterfaceConfigurationEnumeration::GetDHCPEnabledFromConfigData
        (
            lines,
            std::wstring(L"lan666")
        );
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Got wrong value for GetDHCPEnabledFromConfigData", false, actualResult);
    #endif
#endif
    }

    void TestNetInterfaceConfEnumerationFindAll () 
    {
        SCXCoreLib::SCXHandle<NetworkInterfaceDependenciesRethrow> deps(new NetworkInterfaceDependenciesRethrow);
        NetworkInterfaceConfigurationEnumeration networkInterfaceConfigurationEnumeration(deps);

        vector<NetworkInterfaceConfigurationInstance> instances = networkInterfaceConfigurationEnumeration.FindAll();

        int ipEnabledCnt = 0; 

        for(size_t i = 0; i < instances.size(); i++) 
        {
            NetworkInterfaceConfigurationInstance networkInterfaceConfigurationInst = instances[i];
            std::wstring macAddress;
            // Verify the return value.
            CPPUNIT_ASSERT(networkInterfaceConfigurationInst.GetMACAddress(macAddress));
            // Verify the macAddress is not empty.
            CPPUNIT_ASSERT(!macAddress.empty());
            // Check if the interface is enabled.
            bool ipEnabled = false;
            if (networkInterfaceConfigurationInst.GetIPEnabled(ipEnabled) && ipEnabled)
            {
                ipEnabledCnt++;
            }
        }
        // We expect at least one interface to be enabled (UP and RUNNING).
        CPPUNIT_ASSERT(ipEnabledCnt > 0);
     
    }// End of TestNetInterfaceConfEnumerationFindAll() 

    void TestDefaultGatewayIPAddress() 
    {
#if defined(linux)
        SCXCoreLib::SCXHandle<NetworkInterfaceDependenciesDefGatewayIP> deps(new NetworkInterfaceDependenciesDefGatewayIP);
        NetworkInterfaceConfigurationEnumeration networkInterfaceConfigurationEnumeration(deps);

        // Normal operation, we get the default gateway IP.
        vector<NetworkInterfaceConfigurationInstance> instances = networkInterfaceConfigurationEnumeration.FindAll();
        size_t instance_cnt = instances.size();
        for(size_t i = 0; i < instances.size(); i++) 
        {
            NetworkInterfaceConfigurationInstance networkInterfaceConfigurationInst = instances[i];

            std::vector<std::wstring> defaultIPGateway;
            // Verify the return value.
            CPPUNIT_ASSERT(networkInterfaceConfigurationInst.GetDefaultIPGateway(defaultIPGateway));
            // Verify the defaultIPGateway is not empty.
            CPPUNIT_ASSERT(!defaultIPGateway.empty());
            // Verify we have the right address.
            CPPUNIT_ASSERT_EQUAL(deps->DefaultGatewayAddressStr(), StrToUTF8(defaultIPGateway[0]));
        }

        // Force failure.
        deps->ForceDefGatewayIPFailure();
        if(instance_cnt != 0)
        {
            // Test code should catch exception, but only if there actually is an network configuration instance.
            CPPUNIT_ASSERT_THROW(instances = networkInterfaceConfigurationEnumeration.FindAll(),
                    SCXCoreLib::SCXErrnoException);
        }

        // Turn off rethrow. Now exception should be caught inside the provider code and not passed to the
        // test code, but due to failure there should be no default gateway IP.
        deps->EnableRethrow(false);
        instances = networkInterfaceConfigurationEnumeration.FindAll();
        for(size_t i = 0; i < instances.size(); i++) 
        {
            NetworkInterfaceConfigurationInstance networkInterfaceConfigurationInst = instances[i];

            std::vector<std::wstring> defaultIPGateway;
            // Verify that there's no default gateway IP.
            CPPUNIT_ASSERT(!networkInterfaceConfigurationInst.GetDefaultIPGateway(defaultIPGateway));
        }
#elif defined(aix)
        NetworkInterfaceConfigurationEnumeration networkInterfaceConfigurationEnumeration;
        std::vector<NetworkInterfaceConfigurationInstance> instances = networkInterfaceConfigurationEnumeration.FindAll();
        size_t instance_cnt = instances.size();
        CPPUNIT_ASSERT_MESSAGE("Could not find a network interface configuration instance", instance_cnt > (size_t) 0 );
        for(size_t i = 0; i < instances.size(); i++) 
        {
            NetworkInterfaceConfigurationInstance networkInterfaceConfigurationInst = instances[i];

            // On AIX, this calls '/etc/route -n get gateway', which can fail
            // based on system config. Just call for coverage (ensure no core)

            std::vector<std::wstring> defaultIPGateway;
            networkInterfaceConfigurationInst.GetDefaultIPGateway(defaultIPGateway);
        }
#endif
    }

    void TestNetInterfaceIPAddress()
    {
        NetworkInterfaceConfigurationEnumeration networkInterfaceConfigurationEnumeration;

        vector<NetworkInterfaceConfigurationInstance> instances = networkInterfaceConfigurationEnumeration.FindAll();
        if (instances.size() == 0)
        {
            // No network detected on this machine, nothing to do.
            return;
        }
        NetworkInterfaceConfigurationInstance inst = instances[0];
        wstring ifName = inst.GetName();
        vector<wstring> gettedIpAddrs;
        CPPUNIT_ASSERT(inst.GetIPAddress(gettedIpAddrs));
        set<wstring> IPaddrSet; 
        getIPAddrfromIfconfig(ifName, IPaddrSet);
#if defined(hpux)
        for (size_t j = 1; j < gettedIpAddrs.size(); j++)
        {
            wstring ifName1 = ifName + L':' + SCXCoreLib::StrFrom(j);
            getIPAddrfromIfconfig(ifName1, IPaddrSet);
        }
#endif
        CPPUNIT_ASSERT_EQUAL(IPaddrSet.size(), gettedIpAddrs.size());
        set<wstring>::iterator itr;
        for(unsigned int i=0; i<gettedIpAddrs.size(); i++)
        {
            itr = IPaddrSet.find(gettedIpAddrs[i]);
            CPPUNIT_ASSERT(itr != IPaddrSet.end());
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXNetworkInterfaceConfigurationTest ); /* CUSTOMIZE: Name must be same as classname */
