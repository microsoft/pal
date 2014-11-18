/*------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file      scxgateway.h

\brief     Platform independent gateway information

\date      2012-01-09 12:00:00

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXGATEWAY_H
#define SCXGATEWAY_H

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <sys/socket.h>
#include <net/if.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxsystemlib/networkinterface.h>

namespace SCXSystemLib
{
    class GatewayInfo
    {
#if defined(linux)
        /*----------------------------------------------------------------------------*/
        /**
            Reads from socket a gateway IP address.

            \param[in] sock - Socket file descriptor.
            \param[in] msgSeq - Sequence number of messages to be read.
            \param[in] log - Log handle.
            \param[in] deps - Network interface dependencies.
            \param[out] gatewayIP - on success, string containing gateway IP address.

            \returns true if successful, false otherwise.
        */
        static bool recvGatewayIP(int sock, unsigned int msgSeq, SCXCoreLib::SCXLogHandle log,
                SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps, std::wstring& gatewayIP);
#endif
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Retrieves the default gateway IP address.
            
            \param[out] gatewayIP - Gateway Ip address.
            \param deps - Network interface dependencies.
            
            \returns 1 if successful, 0 otherwise.
        */
        static int get_gatewayip(std::wstring & gatewayIP, SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps);
        
        /*----------------------------------------------------------------------------*/
        /**
            Extracts the default gateway IP address from the command "route -n get gateway"
            In this example output, the function would return 10.200.8.129
               route to: 10.172.8.38
            destination: default
                   mask: default
                gateway: 10.200.8.129
              interface: en0
            interf addr: 10.200.8.173
                  flags: <UP,GATEWAY,DONE>
            
            \param[out] gatewayIP - Gateway Ip address.
            \param cmdStringRoute - The exact command to execute
            
            \returns true if successful, false otherwise.
        */
        static bool extract_gatewayip(std::wstring & gatewayIP, std::wstring cmdStringRoute);
    };
}

#endif
