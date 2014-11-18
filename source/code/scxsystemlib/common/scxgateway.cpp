/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
    \file

    \brief     Platform independent network interface

    \date      2012-01-09 12:00:00

 */
/*----------------------------------------------------------------------------*/
 
#include <scxsystemlib/scxgateway.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/logsuppressor.h>
#include <errno.h>
#if defined(linux)
#if PF_MAJOR <=4
#include <linux/types.h>
#endif
#include <linux/rtnetlink.h>
#endif

// Set this variable to true in order to view and debug network socket messages.
static const bool s_fNetworkSocketDebug = false;

using namespace SCXCoreLib;
namespace SCXSystemLib
{
#if defined(linux)
    void DumpNetworkMsg(char *buff, ssize_t len)
    {
        std::cout<<"---------------------------------------------"<<std::endl;
        struct nlmsghdr *nlmsg = (struct nlmsghdr *)buff;
        size_t i = 0;
        for (; NLMSG_OK(nlmsg, ((uint)len)); nlmsg = NLMSG_NEXT(nlmsg, len))
        {
            std::cout<<"  "<<i<<";";
            std::cout<<"nlmsg="<<nlmsg<<";";
            std::cout<<"nlmsg_len="<<nlmsg->nlmsg_len<<";";
            std::cout<<"nlmsg_seq="<<nlmsg->nlmsg_seq<<";";
            std::cout<<"nlmsg_pid="<<nlmsg->nlmsg_pid<<";";
            std::cout<<"nlmsg_flags="<<nlmsg->nlmsg_flags;
            if(nlmsg->nlmsg_flags & NLM_F_MULTI)
            {
               std::cout<<" NLM_F_MULTI";
            }
            std::cout<<";";
            if(nlmsg->nlmsg_type == RTM_NEWROUTE)
            {
                std::cout<<"nlmsg_type=RTM_NEWROUTE"<<";";
                struct rtmsg *rt = static_cast<struct rtmsg *>(NLMSG_DATA(nlmsg));
                std::cout<<"rtm_type="<<static_cast<int>(rt->rtm_type)<<";";
            }
            else
            {
                std::cout<<"nlmsg_type="<<nlmsg->nlmsg_type<<";";
            }
            std::cout<<std::endl;
            i++;
        }
        std::cout<<"---------------------------------------------"<<std::endl;
    }
    bool GatewayInfo::recvGatewayIP(int sock, unsigned int msgSeq, SCXLogHandle log,
            SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps, std::wstring& gatewayIP)
    {
        // Message buffer for recv() call.
        std::vector<char> recvBuffer;
        recvBuffer.resize(1024); // 1k buffer size, a reasonable initial guess.
        
        // Receive messages from the socket until we get gateway IP or we reach the end of messages.
        while (true)
        {
            // Determine the required recv buffer size.
            while (true)
            {
                ssize_t recvPeekSize = deps->recv(sock, &recvBuffer[0], recvBuffer.size(), MSG_PEEK | MSG_DONTWAIT);
                if (recvPeekSize < 0)
                {
                    throw SCXErrnoException(
                        L"GatewayIP socket recv(MSG_PEEK) failed when trying to determine the right buffer size.",
                        errno, SCXSRCLOCATION);
                }
                if (static_cast<size_t>(recvPeekSize) >= recvBuffer.size())
                {
                    if (recvBuffer.size() >= (1024 * 1024))         
                    {
                        // 1 MB and still not enough?!
                        throw SCXInternalErrorException(
                                L"GatewayIP socket recv(MSG_PEEK) asking for unreasonable buffer size, more than 1 MB.",
                                SCXSRCLOCATION);
                    }
                    recvBuffer.resize(recvBuffer.size() * 2);
                }
                else
                {
                    // We have a buffer big enough to receive the entire message.
                    break;
                }
            }

            // Receive the messages from the kernel.
            ssize_t recvLength = deps->recv(sock, &recvBuffer[0], recvBuffer.size(), MSG_DONTWAIT);

            if (recvLength < 0)
            {
                throw SCXErrnoException(L"GatewayIP socket recv() failed to get the message: ", errno, SCXSRCLOCATION);
            }
            if (static_cast<size_t>(recvLength) >= recvBuffer.size())
            {
                throw SCXInternalErrorException(
                        L"GatewayIP socket recv() asking for bigger buffer size, than recv(MSG_PEEK) detected.",
                        SCXSRCLOCATION);
            }
            if (s_fNetworkSocketDebug)
            {
                DumpNetworkMsg(&recvBuffer[0], recvLength);
            }

            // Message pointer initialization.
            struct nlmsghdr *recvMessage;
            recvMessage = (struct nlmsghdr *)&recvBuffer[0];
            
            // We are ready to parse received messages in the buffer.
            // Note:
            // We are supposed to receive NLM_F_MULTI messages terminated by NLMSG_DONE, but that's not how the
            // old linuxes like suse9 behave. Messages are not NLM_F_MULTI but we still receive final NLMSG_DONE, so 
            // we still use NLMSG_DONE to terminate our message parsing but we ignore the NLM_F_MULTI flag. This way
            // the code works on every linux version.
            for (; recvLength > 0; recvMessage = NLMSG_NEXT(recvMessage, recvLength))
            {
                if (NLMSG_OK(recvMessage, ((uint)recvLength)) == false)
                {
                    throw SCXInternalErrorException(L"GatewayIP socket received invalid message from the kernel.",
                            SCXSRCLOCATION);
                }
                if (recvMessage->nlmsg_type == NLMSG_ERROR)
                {
                    struct nlmsgerr *errorMessage = static_cast<struct nlmsgerr *>(NLMSG_DATA(recvMessage));
                    if(errorMessage->error != 0)
                    {
                        throw SCXErrnoException(
                                L"GatewayIP socket received error message: ", errorMessage->error, SCXSRCLOCATION);
                    }
                }
                if (recvMessage->nlmsg_seq != msgSeq)
                {
                    throw SCXInternalErrorException(L"GatewayIP socket received message with wrong msgSeq.",
                            SCXSRCLOCATION);
                }
                if (recvMessage->nlmsg_type == NLMSG_DONE)
                {
                    static LogSuppressor suppressor(eError, eTrace);
                    std::wstring msg = L"Kernel did not provide the gateway IP address.";
                    SCX_LOG(log, suppressor.GetSeverity(msg), msg);
                    // End and we still do not have gateway IP.
                    return false;
                }
                
                if(recvMessage->nlmsg_type == RTM_NEWROUTE ||
                        recvMessage->nlmsg_type == RTM_GETROUTE)
                {
                    // We have a routing message, parse the attributes.
                    struct rtmsg *routeMessage = static_cast<struct rtmsg *>(NLMSG_DATA(recvMessage));
                    if (routeMessage->rtm_family != AF_INET || routeMessage->rtm_table != RT_TABLE_MAIN)
                    {
                        continue;// Wrong message.
                    }
                    // Get the attributes payload.
                    struct rtattr *routeAttribute = RTM_RTA(routeMessage);
                    ssize_t routeAttributeLen = RTM_PAYLOAD(recvMessage);
                    // Find if destination address and gateway address are available.
                    in_addr destinationAddress;
                    destinationAddress.s_addr = INADDR_ANY;// If RTA_DST attribute is not found 0.0.0.0 is assumed.
                    in_addr gatewayAddress;
                    gatewayAddress.s_addr = INADDR_ANY;// If RTA_GATEWAY attribute is not found 0.0.0.0 is assumed.
                    while (RTA_OK(routeAttribute, routeAttributeLen))
                    {
                        switch (routeAttribute->rta_type)
                        {
                        case RTA_DST:
                            destinationAddress.s_addr = *static_cast<in_addr_t *>(RTA_DATA(routeAttribute));
                            break;
                        case RTA_GATEWAY:
                            gatewayAddress.s_addr = *static_cast<in_addr_t *>(RTA_DATA(routeAttribute));
                            break;
                        default:
                            break;
                        }
                        routeAttribute = RTA_NEXT(routeAttribute, routeAttributeLen);
                    }

                    if (destinationAddress.s_addr == INADDR_ANY) // INADDR_ANY == 0.0.0.0, default gateway.
                    {
                        // We have the default gateway address.
                        gatewayIP = SCXCoreLib::StrFromUTF8(inet_ntoa(gatewayAddress));
                        return true;
                    }
                }
            }
        }
        throw SCXInternalErrorException(L"Parsed all the messages from GatewayIP socket and did not receive NLMSG_DONE.",
                SCXSRCLOCATION);
    }
#endif

    int GatewayInfo::get_gatewayip(std::wstring& gatewayIP, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps)
    {
#if defined(linux)
        SCXLogHandle log;
        log = SCXLogHandleFactory::GetLogHandle(
            std::wstring(L"scx.core.common.pal.networkinterfaceconfiguration.scxgateway"));

        class AutoSocket
        {
            int m_sock;
            SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> m_deps;
        public:
            AutoSocket(int sock, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> depsInit):
                    m_sock(sock), m_deps(depsInit)
            {
            }
            ~AutoSocket()
            {
                if(m_sock != -1)
                {
                    m_deps->close(m_sock);
                }
            }
            int GetSock()
            {
                return m_sock;
            }
        };
        try
        {
            gatewayIP.clear();

            // Open socket.
            AutoSocket sock(deps->socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE), deps);
            if (sock.GetSock() < 0)
            {
                throw SCXErrnoException(L"Failed to create socket.", errno, SCXSRCLOCATION);
            }
            
            // Prepare message.
            char sendMessage[NLMSG_LENGTH(sizeof(struct rtmsg))];
            memset(sendMessage, 0, sizeof(sendMessage));
            // Prepare open message header.
            struct nlmsghdr* sendMessageHeader = reinterpret_cast<struct nlmsghdr*>(sendMessage);        
            sendMessageHeader->nlmsg_len = sizeof(sendMessage);// Message length.
            sendMessageHeader->nlmsg_type = RTM_GETROUTE;// Get information about network route.
            sendMessageHeader->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;// Request all entries that match.
            sendMessageHeader->nlmsg_seq = 0;// Message sequence.
            struct rtmsg *sendRouteMessage = static_cast<struct rtmsg *>(NLMSG_DATA(sendMessageHeader));
            sendRouteMessage->rtm_table = RT_TABLE_MAIN;
            sendRouteMessage->rtm_family = AF_INET;
            sendRouteMessage->rtm_type = RTN_UNICAST;

            // Send request to the socket.
            if (deps->send(sock.GetSock(), sendMessageHeader, sendMessageHeader->nlmsg_len, 0) < 0)
            {
                throw SCXErrnoException(L"Failed to send request to the socket.", errno, SCXSRCLOCATION);
            }

            // Read response data from the socket.
            return recvGatewayIP(sock.GetSock(), sendMessageHeader->nlmsg_seq, log, deps, gatewayIP);
        }
        catch(SCXException& e)
        {
            static LogSuppressor suppressor(eError, eTrace);
            std::wstring msg = e.What() + L" " + e.Where();
            SCX_LOG(log, suppressor.GetSeverity(msg), msg);
            if(deps->ShouldRethrow())
            {
                throw;
            }
            gatewayIP.clear();
            return 0;
        }
#elif defined(sun)
        int found_gatewayip = 0;
        std::vector<std::wstring> lines;
        SCXStream::NLFs nlfs;

        SCXFile::ReadAllLines(SCXFilePath(L"/etc/defaultrouter"), lines, nlfs);
        gatewayIP.erase();
        // Global zone default gateway and other zone default gateways should be here
        for (int i = 0; i < lines.size(); i++)
        {
            if (lines[i].find_first_of(L"#") != 0) // ignore comment
            {
                gatewayIP.append(lines[i]);
                found_gatewayip = 1; 
            }
        }

        if (found_gatewayip == 0)
        {
#if PF_MAJOR == 5 && PF_MINOR  == 9
            std::wstring cmdStringRoute = L"/usr/sbin/route -n get gateway";
#elif PF_MAJOR == 5 && (PF_MINOR == 10 || PF_MINOR  == 11)
            std::wstring cmdStringRoute = L"/sbin/route -n get gateway";
#else
#error "Platform not supported"
#endif
            found_gatewayip = GatewayInfo::extract_gatewayip(gatewayIP, cmdStringRoute) ? 1 : 0;
        }
        return found_gatewayip;
#elif defined(aix)
        std::wstring cmdStringRoute = L"/etc/route -n get gateway";
        return GatewayInfo::extract_gatewayip(gatewayIP, cmdStringRoute) ? 1 : 0;
#else
        return 0;
#endif
    }

    bool GatewayInfo::extract_gatewayip(std::wstring & gatewayIP, std::wstring cmdStringRoute)
    {
        std::stringstream strIn; 
        std::stringstream strOut, strErr;
        if (SCXCoreLib::SCXProcess::Run(cmdStringRoute, strIn, strOut, strErr) == 0)
        {
            std::string line; 
            while (strOut.good())
            {
                getline(strOut, line); 
                size_t pos = line.find("gateway"); 
                if(pos == std::string::npos)
                    continue; 

                // Line with "gateway" might have space before colon ... 
                pos = line.find(':', pos); 
                if(pos == std::string::npos)
                    continue; 
                
                // Ensure pos + 1 is valid char
                if(line.length() <= pos)
                    continue; 

                gatewayIP = std::wstring(SCXCoreLib::StrFromUTF8(line.substr(pos + 1)));
                gatewayIP = StrTrim(gatewayIP);
                return true;
            }
        }
        return false;
    }

}
