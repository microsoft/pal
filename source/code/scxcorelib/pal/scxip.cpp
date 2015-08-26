/****************************************************************/
 /**  @file     scxip.cpp
  *   @brief    Helper functions for IP Addresses
 *
 *    @detailed Has unctions to check if an ip address is valid,
 *         to check if a hex value is valid,
 *         and to handle conversion from hex to quad dotted ip format.
 *
****************************************************************/

#include <scxcorelib/scxip.h>
#include <scxcorelib/scxregex.h>
#include <scxcorelib/stringaid.h>

#if defined(SCX_UNIX)
#include <arpa/inet.h>
#include <sstream>
#include <scxcorelib/stringaid.h>
#endif

#if defined(aix) || defined(hpux)
#define _OE_SOCKETS
#include <sys/types.h>
#include <sys/socket.h>
#endif

namespace SCXCoreLib
{
    /**
     *  @brief   A function to check if an IPv4 or an IPv6 address is valid.
     *  @param   ipAddress An ipV4 or ipV6 address
     *  @returns int   1 on success, 0 on failure.
    */
    int IP::IsValidIPAddress(const char *ipAddress)
    {
#if defined (SCX_UNIX)
        struct sockaddr_in sa;
        struct sockaddr_in6 sa6;

        return inet_pton(AF_INET, ipAddress , &(sa.sin_addr)) || inet_pton(AF_INET6, ipAddress, &(sa6.sin6_addr));
#else
        #error "This is not implemented on this platform"
#endif
    }

    /**
     *  @brief   A function to check if an IPv4 or an IPv6 address is valid.
     *  @param   ipAddress An ipV4 or ipV6 address
     *  @returns int   1 on success, 0 on failure.
     *  @note Hex letters can be uppercase or lowercase.
    */
    int IP::IsValidHexAddress(const char *hexAddress)
    {
#if defined (SCX_UNIX)
        std::wstringstream ss;
        ss << hexAddress;

        SCXCoreLib::SCXRegex regex(L"^[0-9A-Fa-f]{8}$");
        return regex.IsMatch(ss.str());
#else
        #error "This is not implemented on this platform"
#endif
    }

   /**
     *  @brief   A function to convert an ipv4 address from hex to quad dot format
     *  @param   hex An ip address wstring in hex format, ie 123ABCDE, without leading 0x.
     *  @returns A string ip address in quad-dotted format
     *  @throws SCXCoreLib::SCXInvalidArgumentException
    */
    std::wstring IP::ConvertHexToIpAddress(std::wstring hex)
    {
#if defined (SCX_UNIX)
        if (!IP::IsValidHexAddress(StrToUTF8(hex).c_str()))
        {
            throw SCXCoreLib::SCXInvalidArgumentException(hex, L"not a valid hex number", SCXSRCLOCATION);
        }
        std::stringstream ss;
        ss << StrToUTF8(hex);
        uint32_t val;
        ss  >> std::hex >>  val;
        struct in_addr addr;
        addr.s_addr = htonl(val);
        std::string ipaddress = inet_ntoa(addr);
        return StrFromUTF8(ipaddress);
#else
        #error "This is not implemented on this platform"
#endif
    }

   /**
     *  @brief   A function to convert a quad-dotted ip address to hex
     *  @param   ipAddress An ip address wstring in quad format, ie 123.123.123.123
     *  @returns A string ip address in hex format
     *  @throws SCXCoreLib::SCXInvalidArgumentException
    */
    std::wstring IP::ConvertIpAddressToHex(std::wstring ipAddress)
    {
#if defined (SCX_UNIX)
        if (!IP::IsValidIPAddress(StrToUTF8(ipAddress).c_str()))
        {
            throw SCXCoreLib::SCXInvalidArgumentException(ipAddress, L"not a valid ip address", SCXSRCLOCATION);
        }
        const unsigned bits_per_term = 8;
        const unsigned num_terms = 4;

        std::istringstream ip(StrToUTF8(ipAddress));
        uint32_t packed = 0;
        for (unsigned int i = 0; i < num_terms; ++i)
        {
            unsigned int term;
            ip >> term;
            ip.ignore();
            packed += term << (bits_per_term * (num_terms-i-1));
        }

        std::wstringstream wss;

        // Uppercase the alpha parts of the resulting hex number.
        // The final hex number may be less than 8 chars wide,
        // and we will pad any remaining space with 0's.

        wss << L"00000000" << std::hex << std::uppercase << packed;

        // just get the last 8 characters in the string
        int pos = static_cast<uint32_t>(wss.str().length()) - 8;
        return wss.str().substr(pos);

 #else
        #error "This is not implemented on this platform"
#endif
}
}
