/*------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file      scxdhcplease.cpp

\brief     Platform independent DHCP lease parser

\date      2012-01-09 12:00:00

*/
/*----------------------------------------------------------------------------*/
#include <stdio.h>
#include <sstream>
#include <ostream>
#include <scxsystemlib/scxdhcplease.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxlog.h>

#include <sys/ioctl.h>
#include <locale.h>
/*
#ifdef aix
#include <netinet/ip.h>
#else
#include <inet/ip.h>
#endif
*/
/**
Constructor.

Sets up the query boilerplate

\param giAddr The DHCP relay IP address octet
\param dhcpServer The DHCP server address octet
*/
namespace SCXSystemLib
{

/*
      Example output that ParseDHCP will parse correctly:

      lease {
              interface \"eth0\";
              fixed-address 10.217.5.146;
              option subnet-mask 255.255.254.0;
              option routers 10.217.4.1;
              option dhcp-lease-time 43200;
              option dhcp-message-type 5;
              option domain-name-servers 10.217.2.6,10.195.172.6;
              option dhcp-server-identifier 10.217.2.6;
              option dhcp-renewal-time 21600;
              option dhcp-rebinding-time 37800;
              option domain-name \"SCX.com\";
              renew 3 2012/04/25 01:36:19;
              rebind 3 2012/04/25 06:55:00;
              expire 3 2012/04/25 08:25:00;
            }
      lease {
              interface \"lan0\";
              fixed-address 10.217.5.146;
              option subnet-mask 255.255.254.0;
              option routers 10.217.4.1;
              option dhcp-lease-time 43200;
              option dhcp-message-type 5;
              option domain-name-servers 10.217.2.6,10.195.172.6;
              option dhcp-server-identifier 10.217.2.6;
              option dhcp-renewal-time 21600;
              option dhcp-rebinding-time 37800;
              option domain-name \"SCX.com\";
              renew 3 2012/04/25 07:11:59;
              rebind 3 2012/04/25 12:06:19;
              expire 3 2012/04/25 13:36:19;
            }
    */
    void DHCPLeaseInfo::ParseDHCP(const std::vector<std::wstring> & lines)
    {
        std::vector<std::wstring> tokens;
        for (int i = 0; i < (int) lines.size(); i++)
        {
            StrTokenize(lines[i], tokens, L" ;");
            // Skip possible blank line
            if (tokens.size() <= 0)
            {
                continue;
            }
            if (tokens[0].compare(L"interface") == 0)
            {
                // Skip bad interface line
                if (tokens.size() < 2)
                {
                    continue;
                }
                m_interface = tokens[1].substr(1, tokens[1].length()-2);
            }
            else if (tokens[0].compare(L"fixed-address") == 0)
            {
                m_fixedAddress = tokens[1];
            }
            else if (tokens[0].compare(L"option") == 0)
            {
                if (tokens[1].compare(L"domain-name") == 0)
                {
                    // Skip bad option domain-name line
                    if (tokens.size() < 3)
                    {
                        continue;
                    }
                    m_domainName = tokens[2].substr(1, tokens[2].length()-2);
                }
                else if (tokens[1].compare(L"dhcp-server-identifier") == 0)
                {
                    m_DHCPServer = tokens[2];
                }
                else if (tokens[1].compare(L"routers") == 0)
                {
                    m_defaultGateway = tokens[2];
                }
            }
            else if (tokens[0].compare(L"expire") == 0)
            {
                struct tm date;
                struct tm time;
                // Skip bad expire line
                if (tokens.size() < 4)
                {
                    continue;
                }
                if (strptime(SCXCoreLib::StrToUTF8(tokens[2]).c_str(), "%x", &date) == 0)
                {
                    // If strptime could not determine localized date, default to %Y%m%d
                    date = parseYmd(tokens[2]);
                }
                strptime(SCXCoreLib::StrToUTF8(tokens[3]).c_str(), "%X", &time);
                m_expiration = SCXCalendarTime(date.tm_year+1900, date.tm_mon+1, date.tm_mday, time.tm_hour, time.tm_min, 0, SCXRelativeTime());
            }
            else if (tokens[0].compare(L"renew") == 0)
            {
                struct tm date;
                struct tm time;
                // Skip bad renew line
                if (tokens.size() < 4)
                {
                    continue;
                }
                if (strptime(SCXCoreLib::StrToUTF8(tokens[2]).c_str(), "%x", &date) == 0)
                {
                    // If strptime could not determine localized date, default to %Y%m%d
                    date = parseYmd(tokens[2]);
                }
                strptime(SCXCoreLib::StrToUTF8(tokens[3]).c_str(), "%X", &time);
                m_renew = SCXCalendarTime(date.tm_year+1900, date.tm_mon+1, date.tm_mday, time.tm_hour, time.tm_min, 0, SCXRelativeTime());
            }
        }
    }

    /*
      Example output that ParseDHCPCD will parse correctly:

      IPADDR='10.217.5.79'
      NETMASK='255.255.254.0'
      NETWORK='10.217.4.0'
      BROADCAST='10.217.5.255'
      ROUTES=''
      GATEWAYS='10.217.5.255'
      DNSDOMAIN='redmond.corp.microsoft.com'
      DNSSERVERS='10.200.81.201 10.200.81.202 10.184.232.13 10.184.232.14'
      DHCPSID='10.184.232.100'
      LEASEDFROM='1335018597'
      LEASETIME='619200'
      RENEWALTIME='309600'
      REBINDTIME='541800'
      INTERFACE='lan0'
      CLASSID='dhcpcd 3.2.3'
      CLIENTID='01:00:16:3e:09:d1:95'
      DHCPCHADDR='00:16:3e:09:d1:95'
      NETBIOSNAMESERVER='157.54.14.163,157.59.200.249,157.54.14.154'
    */
    void DHCPLeaseInfo::ParseDHCPCD(const std::vector<std::wstring> & lines)
    {
        std::vector<std::wstring> tokens;
        int posixLeaseFrom = 0;
        int leaseTime = 0;
        int renewalTime = 0;
        for (int i = 0; i < (int) lines.size(); i++)
        {
            StrTokenize(lines[i], tokens, L"='");
            if (tokens[0].compare(L"INTERFACE") == 0)
            {
                m_interface = tokens[1];
            }
            else if (tokens[0].compare(L"DOMAIN") == 0)
            {
                m_domainName = tokens[1];
            }
            else if (tokens[0].compare(L"DNSDOMAIN") == 0)
            {
                m_domainName = tokens[1];
            }
            else if (tokens[0].compare(L"LEASEDFROM") == 0)
            {
                sscanf(StrToUTF8(tokens[1]).c_str(),"%d", &posixLeaseFrom);
            }
            else if (tokens[0].compare(L"LEASETIME") == 0)
            {
                sscanf(StrToUTF8(tokens[1]).c_str(), "%d", &leaseTime);
            }
            else if (tokens[0].compare(L"RENEWALTIME") == 0)
            {
                sscanf(StrToUTF8(tokens[1]).c_str(), "%d", &renewalTime);
            }
            else if (tokens[0].compare(L"DHCPSID") == 0)
            {
                m_DHCPServer = tokens[1];
            }
            else if (tokens[0].compare(L"IPADDR") == 0)
            {
                m_defaultGateway = tokens[1];
            }
        }
        if (posixLeaseFrom != 0 && leaseTime != 0)
        {
            m_expiration = SCXCalendarTime::FromPosixTime(posixLeaseFrom + leaseTime);
        }
        if (posixLeaseFrom != 0 && renewalTime != 0)
        {
            m_renew = SCXCalendarTime::FromPosixTime(posixLeaseFrom + renewalTime);
        }
    }


    DHCPLeaseInfo::DHCPLeaseInfo(std::wstring interfaceName, std::wstring input) :
    m_interface(interfaceName)
    {
        SCXLogHandle m_log;
        m_log = SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.networkinterfaceconfiguration.scxdhcplease"));

        std::vector<std::wstring> lines;
        SCXStream::NLFs nlfs;
        std::wstring leaseFile = L"";
#if defined(PF_DISTRO_REDHAT)
        // input is used for injection unit tests
        if (input.size() != 0)
        {
            leaseFile = input;
        }
        else if (SCXCoreLib::SCXDirectory::Exists(L"/var/lib/dhcp"))
        {
            leaseFile = L"/var/lib/dhcp/dhclient-" + m_interface + L".leases";
        }
        else if (SCXCoreLib::SCXDirectory::Exists(L"/var/lib/dhclient"))
        {
            leaseFile = L"/var/lib/dhclient/dhclient-" + m_interface + L".leases";
        }
        else
        {
            return;
        }
        SCXFilePath path(leaseFile);
        SCXFile::ReadAllLines(path, lines, nlfs);
        ParseDHCP(lines);
#elif defined(PF_DISTRO_SUSE)
        // input is used for injection unit tests
        if (input.size() != 0)
        {
            leaseFile = input;
        }
        else
        {
            leaseFile = L"/var/lib/dhcpcd/dhcpcd-" + m_interface + L".info";
        }
        SCXFile::ReadAllLines(SCXFilePath(leaseFile), lines, nlfs);
        ParseDHCPCD(lines);
#elif defined(PF_DISTRO_ULINUX)
        // input is used for injection unit tests
        if (input.size() != 0)
        {
            SCXFile::ReadAllLines(SCXFilePath(input), lines, nlfs);
            if (StrTrim(lines[0]) == L"lease {")
            {
                // parse input like dhcp
                ParseDHCP(lines);
                return;
            }
            else
            {
                // parse input like dhcpcd
                ParseDHCPCD(lines);
                return;
            }
        }
        else
        {
            std::vector<std::wstring> dhcpLocations;
            dhcpLocations.push_back(L"/var/lib/dhcp/dhclient-" + m_interface + L".leases");
            dhcpLocations.push_back(L"/var/lib/dhcp/dhclient." + m_interface + L".leases");
            dhcpLocations.push_back(L"/var/lib/dhcp3/dhclient." + m_interface + L".leases");
            dhcpLocations.push_back(L"/var/lib/dhclient/dhclient-" + m_interface + L".leases");

            std::vector<std::wstring> dhcpcdLocations;
            dhcpcdLocations.push_back(L"/var/lib/dhcpcd/dhcpcd-" + m_interface + L".info");

            for (std::vector<std::wstring>::iterator it = dhcpLocations.begin(); it != dhcpLocations.end(); it++)
            {
                SCXFile::ReadAllLines(SCXFilePath(*it), lines, nlfs);
                if (lines.size() > 0)
                {
                    // parse input like dhcp
                    ParseDHCP(lines);
                    return;
                }
            }

            for (std::vector<std::wstring>::iterator it = dhcpcdLocations.begin(); it != dhcpcdLocations.end(); it++)
            {
                SCXFile::ReadAllLines(SCXFilePath(*it), lines, nlfs);
                if (lines.size() > 0)
                {
                    // parse input like dhcpcd
                    ParseDHCPCD(lines);
                    return;
                }
            }
        }

#elif defined(sun)
        std::vector<std::wstring> tokens;
        std::istringstream processInput;
        std::ostringstream processOutput;
        std::ostringstream processErr;

        std::wstring cmdDHCP(L"/usr/bin/netstat -D");
        if (input.size() != 0)
        {
            cmdDHCP = input;
        }

        try
        {
            SCXCoreLib::SCXProcess::Run(cmdDHCP, processInput, processOutput, processErr, 15000);
            std::istringstream dhcpResult(processOutput.str());

            lines.clear();
            SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(dhcpResult, lines, nlfs);

            std::wstring errOut = StrFromUTF8(processErr.str());
            for (int i = 0; i < (int) lines.size(); i++)
            {
                StrTokenize(lines[i], tokens);
                if (tokens[0].compare(m_interface) == 0 && i+1 < lines.size())
                {
                    StrTokenize(lines[i+1], tokens, L"(), ");
                    m_expiration = strToSCXCalendarTime(tokens[6], tokens[7]);
                    m_renew      = strToSCXCalendarTime(tokens[8], tokens[9]);
                    break;
                }
            }
        }
        catch (SCXCoreLib::SCXException &e)
        {
            SCX_LOGERROR(m_log, L"Exception on netstat process:" + e.What());
        }
#elif defined(hpux)

        #define IF_NAME             0
        #define DOMAIN_NAME         5
        #define LEASE_DURATION      6
        #define LEASE_EXPIRATION    7
        #define RENEWAL_PERCENT     9
        #define DEFAULT_GATEWAY     15
        #define SERVER_ADDR         16
        #define DNS_SERVERS         19 /* duplicated in resolv.conf */

        std::vector<std::wstring> tokens;
        if (input.size() != 0)
        {
            leaseFile = input;
        }
        else
        {
            leaseFile = L"/etc/dhcpclient.data";
        }
        SCXFile::ReadAllLines(SCXFilePath(leaseFile), lines, nlfs);
        std::vector< std::vector<std::wstring> > parsedLines;

        // For a set of lines of form
        //      <code> <length> f1 f2 f3 ... f<n>, where <code> and <length> are ints
        // put f1 ... f<n> in vector parsedLines[<code>]
        /* For example:
               00 4 lan0
               01 0
               02 0
               03 0
               04 0
               05 7 SCX.com
               06 4 43200
               07 4 1334804239
               08 4 0
               09 4 0
               10 4 1
               11 6 16 aa 19 ff 30 7a
               12 4 10.217.5.127
               13 4 255.255.254.0
               14 4 0.0.0.0
               15 4 10.217.4.1
               16 4 10.217.2.6
               17 4 0.0.0.0
               18 0
               19 8 10.217.2.6 10.195.172.6
               20 0
               21 4 0.0.0.0
               22 0
               23 0
               24 64 63 82 53 63 35 1 5 3a 4 0 0 54 60 3b 4 0 0 93 a8 33 4 0 0 a8 c0 36 4 a d9 ... etc
               00 4 lan1
               01 ...
               ...
               24 5 ab cd ef 12 34
        */

        // <length> seems to be the number of bytesfor a single or list of ip addresses
        //                                            (e.g., two ip addresses is 8 octets)
        //                      the number of space delimited substrings, for a mac address or option list
        //                      the number of characters, for the interface name

        bool foundInterface = false;
        for (int i = 0; i < lines.size(); i++)
        {
            StrTokenize(lines[i], tokens, L" .");
            int code = StrToUInt(tokens[0]);
            int length = StrToUInt(tokens[1]);

            // If we are starting the block for the interface of interest,
            // create a vector to hold the tokens and add the interface as the first token
            if (code == IF_NAME)
            {
                // We're done if we have already processed an interface
                if (foundInterface)
                {
                    break;
                }
                // But if not, check if this is the right interface
                if (tokens[2].compare(m_interface) == 0)
                {
                    // Though we don't actually reference data for parsedLines[IF_NAME]
                    // add its content anyway to keep the element number of parsedLines matching <code>
                    std::vector<std::wstring> lineTokens;
                    lineTokens.push_back(tokens[2]);
                    parsedLines.push_back(lineTokens);
                    foundInterface = true;
                }
                continue;
            }

            // Skip other codes if they belong to the wrong interface
            if (!foundInterface)
            {
                continue;
            }

            // Otherwise, we're at the following codes for the right interface.  Create a vector for this line's tokens.
            std::vector<std::wstring> lineTokens;
            // Save tokens 2 to end
            for (int j = 2; j < tokens.size(); j++)
            {
                lineTokens.push_back(tokens[j]);
            }
            parsedLines.push_back(lineTokens);
        }

        // No member variables can be set if we never found the right interface, so quit.
        if (!foundInterface)
        {
            return;
        }

        // Compute intervals from lease duration and renewal percent
        int leaseDurationSecs = StrToUInt(parsedLines[LEASE_DURATION][0]);
        SCXAmountOfTime leaseDurationInterval;
        leaseDurationInterval.SetSeconds(leaseDurationSecs);
        int renewalPercent = StrToUInt(parsedLines[RENEWAL_PERCENT][0]);
        // Default renewal is 50%
        if (renewalPercent == 0)
        {
            renewalPercent = 50;
        }
        SCXAmountOfTime renewalInterval;
        renewalInterval.SetSeconds(leaseDurationSecs * renewalPercent / 100);

        // Set member variables
        m_expiration = SCXCalendarTime::FromPosixTime(StrToUInt(parsedLines[LEASE_EXPIRATION][0]));
        m_renew = m_expiration - leaseDurationInterval + renewalInterval;
        m_DHCPServer =     strJoin(parsedLines[SERVER_ADDR],     L".");
        m_domainName =     strJoin(parsedLines[DOMAIN_NAME],     L".");
        m_defaultGateway = strJoin(parsedLines[DEFAULT_GATEWAY], L".");
#elif defined(aix)
        //Not implemented
#else
    #error Platform not supported
#endif
    }

    struct tm DHCPLeaseInfo::parseYmd(std::wstring ymd)
    {
        struct tm date;
        std::vector<std::wstring> parsedDate;
        StrTokenize(ymd.c_str(), parsedDate, L" /-");
        int yr;
        date.tm_year = yr = SCXCoreLib::StrToUInt(parsedDate[0]);
        if (yr > 1900)
        {
            date.tm_year = yr - 1900;
        }
        date.tm_mon = SCXCoreLib::StrToUInt(parsedDate[1]) - 1;
        date.tm_mday = SCXCoreLib::StrToUInt(parsedDate[2]);
        return date;
   }

   SCXCalendarTime DHCPLeaseInfo::strToSCXCalendarTime(std::wstring date, std::wstring time)
   {
       struct tm edate;
       // This ugly thing is needed because strptime appears to choke on some legal strings
       // Try US date version, then localized date, then default %Y%m%d
       if (strptime(SCXCoreLib::StrToUTF8(date).c_str(), "%m/%d/%Y", &edate) == 0)
       {
           struct tm edateLoc;
           if (strptime(SCXCoreLib::StrToUTF8(date).c_str(), "%x", &edateLoc) == 0)
           {
               edate = parseYmd(date);
           }
           else
           {
               edate = edateLoc;
           }
       }

       struct tm etime;
       strptime(SCXCoreLib::StrToUTF8(time).c_str(), "%H:%M", &etime); // 24 hour clock

       return SCXCalendarTime(edate.tm_year+1900, edate.tm_mon+1, edate.tm_mday, etime.tm_hour, etime.tm_min, 0, SCXRelativeTime());
   }

   std::wstring DHCPLeaseInfo::strJoin(std::vector<std::wstring> parts, std::wstring sep)
   {
        std::wstring result;
        int nSeparators = (int) parts.size();
        for (int i = 0; i < nSeparators; i++)
        {
            result += parts[i];
            if (i != nSeparators-1)
            {
               result += sep;
            }
        }
        return result;
   }

}
