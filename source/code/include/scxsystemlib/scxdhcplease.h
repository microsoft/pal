/*------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file      scxcdhcplease.h

\brief     Platform independent DHCP lease parser

\date      2012-01-09 12:00:00

\author    a-tiro

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXDHCPLEASE_H
#define SCXDHCPLEASE_H

#include <scxcorelib/stringaid.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <net/if.h>

#include <unistd.h>
#include <arpa/inet.h>

#include <scxcorelib/scxtime.h>

using namespace SCXCoreLib;

namespace SCXSystemLib
{
    class DHCPLeaseInfo
    {
    public:
        DHCPLeaseInfo(std::wstring interfaceName, std::wstring input=L"");
        /*----------------------------------------------------------------------------*/
        /**
            The DHCP lease expiration date

            \returns     The lease expiration date computed in the constructor
        */
        inline SCXCalendarTime const getLeaseExpires() const
        {
            return m_expiration;
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            Date the DHCP lease was made

            \returns     The lease date computed in the constructor
        */
        inline SCXCalendarTime const getLeaseObtained() const
        {
            return m_renew;
        }

        /*----------------------------------------------------------------------------*/
        /**
            The network interface name for this DHCP lease

            \returns     The interface name
        */
        inline std::wstring const getInterface() const
        {
            return m_interface;
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            The DHCP IP address for this network interface

            \returns     The IP address computed in the constructor
        */
        inline std::wstring const getFixedAddress() const
        {
            return m_fixedAddress;
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            The DHCP organization name and extension

            \returns     The domain name computed in the constructor
        */
        inline std::wstring const getDomainName() const
        {
            return m_domainName;
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            The DHCP Server IP address

            \returns     The DHCP server computed in the constructor
        */
        inline std::wstring const getDHCPServer() const
        {
            return m_DHCPServer;
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            The Default Gateway address

            \returns     The Default Gateway computed in the constructor
        */
        inline std::wstring const getDefaultGateway() const
        {
            return m_defaultGateway;
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            Convert text date to tm struct date

            \param ymd - Text date, such as YYYY/mm/dd, YYYY/m/d, or YYYY-mm-dd
            \returns     struct tm version of ymd.
        */
        static struct tm parseYmd(std::wstring ymd);
        
        /*----------------------------------------------------------------------------*/
        /**
            Convert text date and time to SCXCalendarTime

            \param date - date in localized format
            \param time - time in localized format
            \returns     SCXCalendarTime from parsing scxstrings date and time
        */
        static SCXCalendarTime strToSCXCalendarTime(std::wstring date, std::wstring time);
        
    private:
        // The network interface name
        std::wstring m_interface;
        // The organization name and extension
        std::wstring m_domainName;
        // The DHCP IP address
        std::wstring m_fixedAddress;
        // The DHCP server IP addres
        std::wstring m_DHCPServer;
        // The DHCP lease expiration date
        SCXCalendarTime m_expiration;
        // The time the DHCP lease was renewed
        SCXCalendarTime m_renew;
        // Default gateway
        std::wstring m_defaultGateway;
        
        std::wstring strJoin(std::vector<std::wstring> str, std::wstring sep);

        /*----------------------------------------------------------------------------*/
        /**
           Helper function used to parse "lines" as though it was formatted like a file from
           /var/lib/dhcp/dhclient-*

           \param lines - a vector of scxstrings containing the contents of a dhclient-* file.
        */
        void ParseDHCP(const std::vector<std::wstring> & lines);

        /*----------------------------------------------------------------------------*/
        /**
           Helper function used to parse "lines" as though it was formatted like a file from
           /var/lib/dhcpcd/dhcpcd-*

           \param lines - a vector of scxstrings containing the contents of a dhcpcd-* file.
        */
        void ParseDHCPCD(const std::vector<std::wstring> & lines);
    };
}

#endif
